// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */
 #define pr_fmt(fmt) "qcom_mpam: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/configfs.h>
#include <linux/string.h>
#include <linux/bitmap.h>
#include <linux/sched/walt.h>
#include <trace/hooks/mpam.h>
#include <linux/scmi_protocol.h>
#include <linux/qcom_scmi_vendor.h>
#include <soc/qcom/mpam.h>

#define MPAM_ALGO_STR	0x4D50414D4558544E  /* "MPAMEXTN" */

/* Parameter IDs for SET */
enum mpam_set_param_ids {
	PARAM_SET_CACHE_PARTITION = 1,
	PARAM_SET_CONFIG_MONITOR = 2,
	PARAM_SET_CAPTURE_ALL_MONITOR = 3,
};

/* Parameter IDs for GET */
enum mpam_get_param_ids {
	PARAM_GET_MPAM_VERSION = 1,
	PARAM_GET_CACHE_PARTITION = 2
};

struct qcom_mpam_partition {
	struct config_group group;
	int part_id;
	struct mpam_slice_val val[MSC_MAX];
};

static struct scmi_protocol_handle *ph;
static const struct qcom_scmi_vendor_ops *ops;
static struct scmi_device *sdev;
static unsigned long *part_id_free_bitmap;
static struct mpam_slice_val mpam_default_val;

int qcom_mpam_set_cache_partition(struct mpam_set_cache_partition *param)
{
	int ret = -EPERM;

	if (ops)
		ret = ops->set_param(ph, param, MPAM_ALGO_STR,
				PARAM_SET_CACHE_PARTITION,
				sizeof(struct mpam_set_cache_partition));

	return ret;
}
EXPORT_SYMBOL_GPL(qcom_mpam_set_cache_partition);

int qcom_mpam_get_version(struct mpam_ver_ret *ver)
{
	int ret = -EPERM;

	if (ops) {
		ret = ops->get_param(ph, ver, MPAM_ALGO_STR,
				PARAM_GET_MPAM_VERSION, 0,
				sizeof(struct mpam_ver_ret));
	}

	return ret;
}
EXPORT_SYMBOL_GPL(qcom_mpam_get_version);

int qcom_mpam_get_cache_partition(struct mpam_read_cache_portion *param,
						struct mpam_slice_val *val)
{
	int ret = -EPERM;
	uint8_t buf[32];

	if (ops) {
		memcpy(buf, param, sizeof(struct mpam_read_cache_portion));
		ret = ops->get_param(ph, buf, MPAM_ALGO_STR,
				PARAM_GET_CACHE_PARTITION,
				sizeof(struct mpam_read_cache_portion),
				sizeof(struct mpam_slice_val));
	}

	if (!ret)
		memcpy(val, buf, sizeof(struct mpam_slice_val));

	return ret;
}
EXPORT_SYMBOL_GPL(qcom_mpam_get_cache_partition);

static inline struct qcom_mpam_partition *to_partition(
					   struct config_item *item)
{
	return container_of(to_config_group(item),
				struct qcom_mpam_partition, group);
}

static inline int get_part_id(struct config_item *item)
{
	return to_partition(item)->part_id;
}

static inline void set_part_id(struct config_item *item,
				int part_id)
{
	to_partition(item)->part_id = part_id;
}

static void qcom_mpam_partition_transfer(int old, int new)
{
	struct task_struct *p, *t;
	struct walt_task_struct *wts;

	if (old == new)
		return;

	rcu_read_lock();
	for_each_process_thread(p, t) {
		wts = (struct walt_task_struct *) t->android_vendor_data1;
		if (wts->mpam_part_id == old)
			wts->mpam_part_id = new;
	}
	rcu_read_unlock();
}

static ssize_t qcom_mpam_part_id_show(struct config_item *item, char *page)
{
	return scnprintf(page, PAGE_SIZE, "%d\n", get_part_id(item));
}
CONFIGFS_ATTR_RO(qcom_mpam_, part_id);

/*
 * Schemata supports any combination of mpam parameter settings.
 * Each parameter is a key-value pair separated by equal sign,
 * and multiple parameters are separated by a comma.
 * E.g. cmax=50 / cmax=50,prio=0
 */
static void qcom_mpam_set_schemata(struct config_item *item,
		char *buf, enum msc_id mscid)
{
	int ret;
	uint32_t input;
	bool bypass_cache = false;
	char *token, *param_name;
	struct mpam_set_cache_partition mpam_param;
	struct qcom_mpam_partition *partition = to_partition(item);

	mpam_param.msc_id = mscid;
	mpam_param.part_id = get_part_id(item) + PARTID_RESERVED;
	mpam_param.cache_capacity = partition->val[mscid].capacity;
	mpam_param.cpbm_mask = partition->val[mscid].cpbm;
	mpam_param.dspri = partition->val[mscid].dspri;
	mpam_param.mpam_config_ctrl = SET_CACHE_CAPACITY_AND_CPBM_AND_DSPRI;

	while ((token = strsep(&buf, ",")) != NULL) {
		param_name = strsep(&token, "=");
		if (param_name == NULL || token == NULL)
			continue;
		if (kstrtouint(token, 0, &input) < 0) {
			pr_err("invalid argument for %s\n", param_name);
			continue;
		}

		if (!strcmp("cmax", param_name))
			mpam_param.cache_capacity = input;
		else if (!strcmp("cpbm", param_name))
			mpam_param.cpbm_mask = input;
		else if (!strcmp("prio", param_name))
			mpam_param.dspri = input;
	}

	/*
	 * If cpbm_mask set from userspace is 0, it means the least allocation
	 * for MPAM is needed. For the current hardware, the least allocation
	 * is cpbm_mask==1, cache_capacity == 0. And when cpbm_mask == 0,
	 * cache_capacity will always limit to 0.
	 */
	if (mpam_param.cpbm_mask == 0) {
		bypass_cache = true;
		mpam_param.cpbm_mask = 0x1;
		mpam_param.cache_capacity = 0;
	}

	ret = qcom_mpam_set_cache_partition(&mpam_param);
	if (!ret) {
		partition->val[mscid].capacity = mpam_param.cache_capacity;
		partition->val[mscid].dspri = mpam_param.dspri;
		if (unlikely(bypass_cache))
			partition->val[mscid].cpbm = 0;
		else
			partition->val[mscid].cpbm = mpam_param.cpbm_mask;
	} else
		pr_err("set msc mpam settings failed, ret = %d\n", ret);
}

static ssize_t qcom_mpam_schemata_0_show(struct config_item *item,
		char *page)
{
	struct qcom_mpam_partition *partition = to_partition(item);
	struct mpam_slice_val *mpam_val = &partition->val[MSC_0];

	return scnprintf(page, PAGE_SIZE, "cmax=%d,cpbm=0x%x,prio=%d\n",
		mpam_val->capacity, mpam_val->cpbm, mpam_val->dspri);
}

static ssize_t qcom_mpam_schemata_0_store(struct config_item *item,
		const char *page, size_t count)
{
	qcom_mpam_set_schemata(item, (char *)page, MSC_0);
	return count;
}
CONFIGFS_ATTR(qcom_mpam_, schemata_0);

static ssize_t qcom_mpam_schemata_1_show(struct config_item *item,
		char *page)
{
	struct qcom_mpam_partition *partition = to_partition(item);
	struct mpam_slice_val *mpam_val = &partition->val[MSC_1];

	return scnprintf(page, PAGE_SIZE, "cmax=%d,cpbm=0x%x,prio=%d\n",
		mpam_val->capacity, mpam_val->cpbm, mpam_val->dspri);
}

static ssize_t qcom_mpam_schemata_1_store(struct config_item *item,
		const char *page, size_t count)
{
	qcom_mpam_set_schemata(item, (char *)page, MSC_1);
	return count;
}
CONFIGFS_ATTR(qcom_mpam_, schemata_1);

static ssize_t qcom_mpam_tasks_show(struct config_item *item, char *page)
{
	int part_id;
	ssize_t len = 0;
	struct task_struct *p, *t;
	struct walt_task_struct *wts;

	part_id = get_part_id(item);
	rcu_read_lock();
	for_each_process_thread(p, t) {
		wts = (struct walt_task_struct *) t->android_vendor_data1;
		if (wts->mpam_part_id == part_id)
			len += scnprintf(page + len, PAGE_SIZE - len, "%d ", t->pid);
	}
	rcu_read_unlock();
	len += scnprintf(page + len, PAGE_SIZE - len, "\n");

	return len;
}

static ssize_t qcom_mpam_tasks_store(struct config_item *item,
		const char *page, size_t count)
{
	int ret, part_id;
	pid_t pid_input;
	char *kbuf, *token;
	struct task_struct *p;
	struct walt_task_struct *wts;

	part_id = get_part_id(item);
	kbuf = (char *)page;
	while ((token = strsep(&kbuf, " ")) != NULL) {
		ret = kstrtouint(token, 10, &pid_input);
		if (ret < 0) {
			pr_err("invalid argument\n");
			goto err;
		}

		p = find_task_by_vpid(pid_input);
		if (IS_ERR_OR_NULL(p)) {
			pr_err("pid %d not exist\n", pid_input);
			continue;
		}

		wts = (struct walt_task_struct *) p->android_vendor_data1;
		wts->mpam_part_id = part_id;
	}

err:
	return count;
}
CONFIGFS_ATTR(qcom_mpam_, tasks);

static struct configfs_attribute *qcom_mpam_attrs[] = {
	&qcom_mpam_attr_part_id,
	&qcom_mpam_attr_schemata_0,
	&qcom_mpam_attr_schemata_1,
	&qcom_mpam_attr_tasks,
	NULL,
};

static void qcom_mpam_reset_param(int part_id)
{
	struct mpam_set_cache_partition mpam_param;

	mpam_param.part_id = part_id;
	mpam_param.msc_id = 0;
	mpam_param.dspri = mpam_default_val.dspri;
	mpam_param.cpbm_mask = mpam_default_val.cpbm;
	mpam_param.cache_capacity = mpam_default_val.capacity;
	qcom_mpam_set_cache_partition(&mpam_param);

	mpam_param.msc_id = 1;
	qcom_mpam_set_cache_partition(&mpam_param);
}

static void qcom_mpam_drop_item(struct config_group *group,
		struct config_item *item)
{
	int part_id;

	part_id = get_part_id(item);

	qcom_mpam_partition_transfer(part_id, PARTID_DEFAULT);
	bitmap_clear(part_id_free_bitmap, part_id, 1);
	qcom_mpam_reset_param(part_id);

	kfree(to_partition(item));
}

static const struct config_item_type qcom_mpam_item_type = {
	.ct_attrs	= qcom_mpam_attrs,
};

static struct config_group *qcom_mpam_make_group(
		struct config_group *group, const char *name)
{
	int i, part_id;
	struct qcom_mpam_partition *partition;

	part_id = bitmap_find_next_zero_area(part_id_free_bitmap,
				   PARTID_AVAILABLE, 0, 1, 0);

	if (part_id > PARTID_AVAILABLE)
		return ERR_PTR(-ENOMEM);

	partition = kzalloc(sizeof(struct qcom_mpam_partition), GFP_KERNEL);
	if (!partition)
		return ERR_PTR(-ENOMEM);

	bitmap_set(part_id_free_bitmap, part_id, 1);
	partition->part_id = part_id;
	for (i = 0; i < MSC_MAX; i++)
		memcpy(&(partition->val[i]), &mpam_default_val, sizeof(struct mpam_slice_val));
	qcom_mpam_reset_param(part_id);

	config_group_init_type_name(&partition->group, name,
				   &qcom_mpam_item_type);

	return &partition->group;
}

static struct configfs_group_operations qcom_mpam_group_ops = {
	.make_group	= qcom_mpam_make_group,
	.drop_item	= qcom_mpam_drop_item,
};

static const struct config_item_type qcom_mpam_subsys_type = {
	.ct_group_ops	= &qcom_mpam_group_ops,
	.ct_owner	= THIS_MODULE,
};

static struct configfs_subsystem qcom_mpam_subsys = {
	.su_group = {
		.cg_item = {
			.ci_namebuf = "qcom_mpam",
			.ci_type = &qcom_mpam_subsys_type,
		},
	},
};

static void qcom_mpam_write_partid(u8 part_id, pid_t next_pid)
{
	u64 reg;

	part_id += PARTID_RESERVED;
	reg = (part_id << PARTID_I_SHIFT) | (part_id << PARTID_D_SHIFT);

	write_sysreg_s(reg, SYS_MPAM0_EL1);
	write_sysreg_s(reg, SYS_MPAM1_EL1);
}

static void qcom_mpam_switch_task(void *unused, struct task_struct *prev,
							struct task_struct *next)
{
	struct walt_task_struct *wts;

	wts = (struct walt_task_struct *) next->android_vendor_data1;
	qcom_mpam_write_partid(wts->mpam_part_id, next->pid);
}

static int qcom_mpam_configfs_init(void)
{
	int ret;
	struct config_group *default_group;

	part_id_free_bitmap = bitmap_zalloc(PARTID_AVAILABLE, GFP_KERNEL);
	if (!part_id_free_bitmap) {
		pr_err("Error alloc bitmap\n");
		return -ENOMEM;
	}

	config_group_init(&qcom_mpam_subsys.su_group);
	mutex_init(&qcom_mpam_subsys.su_mutex);

	default_group = qcom_mpam_make_group(NULL, "default");
	configfs_add_default_group(default_group, &qcom_mpam_subsys.su_group);

	ret = configfs_register_subsystem(&qcom_mpam_subsys);
	if (ret) {
		mutex_destroy(&qcom_mpam_subsys.su_mutex);
		pr_err("Error while registering subsystem %d\n", ret);
		return ret;
	}

	register_trace_android_vh_mpam_set(qcom_mpam_switch_task, NULL);

	return 0;
}

static void qcom_mpam_configfs_remove(void)
{
	configfs_unregister_subsystem(&qcom_mpam_subsys);
	unregister_trace_android_vh_mpam_set(qcom_mpam_switch_task, NULL);
}

static int qcom_mpam_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mpam_read_cache_portion mpam_param;

	sdev = get_qcom_scmi_device();
	if (IS_ERR(sdev)) {
		ret = PTR_ERR(sdev);
		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev, "Error getting scmi_dev ret=%d\n", ret);
		return ret;
	}
	ops = sdev->handle->devm_protocol_get(sdev, QCOM_SCMI_VENDOR_PROTOCOL, &ph);
	if (IS_ERR(ops)) {
		ret = PTR_ERR(ops);
		ops = NULL;
		dev_err(&pdev->dev, "Error getting vendor protocol ops: %d\n", ret);
		return ret;
	}

	mpam_param.msc_id = 0;
	mpam_param.part_id = PARTID_MAX - 1;
	ret = qcom_mpam_get_cache_partition(&mpam_param, &mpam_default_val);
	if (ret) {
		dev_err(&pdev->dev, "Error getting default value %d\n", ret);
		mpam_default_val.cpbm = UINT_MAX;
	}
	mpam_default_val.capacity = 100;
	mpam_default_val.dspri = 0;

	if (IS_ENABLED(CONFIG_QTI_MPAM_CONFIGFS)) {
		ret = qcom_mpam_configfs_init();
		if (ret)
			dev_err(&pdev->dev, "Error creating configfs %d\n", ret);
	}

	return ret;
}

static int qcom_mpam_remove(struct platform_device *pdev)
{
	if (IS_ENABLED(CONFIG_QTI_MPAM_CONFIGFS))
		qcom_mpam_configfs_remove();
	return 0;
}

static const struct of_device_id qcom_mpam_table[] = {
	{ .compatible = "qcom,mpam" },
	{}
};

static struct platform_driver qcom_mpam_driver = {
	.driver = {
		.name = "qcom-mpam",
		.of_match_table = qcom_mpam_table,
	},
	.probe = qcom_mpam_probe,
	.remove = qcom_mpam_remove,
};

module_platform_driver(qcom_mpam_driver);

MODULE_SOFTDEP("pre: qcom_scmi_client");
MODULE_DESCRIPTION("QCOM MPAM driver");
MODULE_LICENSE("GPL");
