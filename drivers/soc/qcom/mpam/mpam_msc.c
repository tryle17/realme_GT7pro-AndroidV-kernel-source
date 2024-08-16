// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#define pr_fmt(fmt) "mpam_msc: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/string.h>
#include <linux/qcom_scmi_vendor.h>
#include <linux/scmi_protocol.h>
#include <soc/qcom/mpam_msc.h>
#include <soc/qcom/mpam_slc.h>

static LIST_HEAD(qcom_mpam_list);
#define DELIM_CHAR		" "

struct qcom_mpam_msc *qcom_msc_lookup(uint32_t msc_id)
{
	struct qcom_mpam_msc *mpam_msc;

	list_for_each_entry(mpam_msc, &qcom_mpam_list, node) {
		if (mpam_msc->msc_id == msc_id)
			return mpam_msc;
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(qcom_msc_lookup);

int msc_system_get_partition(uint32_t msc_id, void *arg1, void *arg2)
{
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;

	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->get_cache_partition)
			ret = qcom_msc->ops->get_cache_partition(qcom_msc->dev, arg1, arg2);
		break;
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(msc_system_get_partition);

int msc_system_get_device_capability(uint32_t msc_id, void *arg1, void *arg2)
{
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;

	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->get_cache_partition_capability)
			ret = qcom_msc->ops->get_cache_partition_capability(qcom_msc->dev, arg1, arg2);
		break;
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(msc_system_get_device_capability);

int msc_system_set_partition(uint32_t msc_id, void *arg1, void *arg2)
{
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;

	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->set_cache_partition)
			ret = qcom_msc->ops->set_cache_partition(qcom_msc->dev, arg1, arg2);
		break;
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(msc_system_set_partition);

int msc_system_miss_config(uint32_t msc_id, void *arg1, void *arg2)
{
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;

	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->mon_config)
			ret = qcom_msc->ops->mon_config(qcom_msc->dev, arg1, arg2);
		break;
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(msc_system_miss_config);

int msc_system_reset_partition(uint32_t msc_id, void *arg1, void *arg2)
{
	uint8_t qcom_msc_type;
	struct qcom_mpam_msc *qcom_msc;
	int ret = -EINVAL;

	qcom_msc = qcom_msc_lookup(msc_id);
	if (qcom_msc == NULL)
		return ret;

	qcom_msc_type = qcom_msc->qcom_msc_id.qcom_msc_type;
	switch (qcom_msc_type) {
	case SLC:
		if (qcom_msc->ops->set_cache_partition)
			ret = qcom_msc->ops->reset_cache_partition(qcom_msc->dev, arg1, arg2);
		break;
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(msc_system_reset_partition);

int attach_dev(struct device *dev, struct qcom_mpam_msc *qcom_msc,
		uint32_t msc_type)
{
	int ret = -EINVAL;

	switch (msc_type) {
	case SLC:
		break;
	default:
		return ret;
	}


	list_add_tail(&qcom_msc->node, &qcom_mpam_list);
	return 0;
}
EXPORT_SYMBOL_GPL(attach_dev);

void detach_dev(struct device *dev, struct qcom_mpam_msc *qcom_msc, uint32_t msc_type)
{
	switch (msc_type) {
	case SLC:
		break;
	default:
		return;
	}

	list_del(&qcom_msc->node);
}
EXPORT_SYMBOL_GPL(detach_dev);

static struct attribute_group mpam_slc_sysfs_group = {
	.attrs	= mpam_slc_sysfs_attrs,
};

static int mpam_msc_probe(struct platform_device *pdev)
{
	int ret;

	if (of_platform_populate(pdev->dev.of_node, NULL, NULL, &pdev->dev) < 0)
		dev_err(&pdev->dev, "Mpam driver probe failed.!\n");

	return 0;
}

int mpam_msc_remove(struct platform_device *pdev)
{
	return 0;
}
static const struct of_device_id mpam_msc_table[] = {
	{ .compatible = "qcom,mpam-msc" },
	{}
};

MODULE_DEVICE_TABLE(of, mpam_msc_table);

static struct platform_driver mpam_msc_driver = {
	.driver = {
		.name = "mpam-msc",
		.of_match_table = mpam_msc_table,
	},
	.probe = mpam_msc_probe,
	.remove = mpam_msc_remove,
};

module_platform_driver(mpam_msc_driver);

MODULE_SOFTDEP("pre: mpam");
MODULE_DESCRIPTION("QCOM MPAM MSC driver");
MODULE_LICENSE("GPL");
