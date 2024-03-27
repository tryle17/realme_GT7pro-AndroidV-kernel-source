// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/cpu.h>
#include <linux/slab.h>
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

static struct scmi_protocol_handle *ph;
static const struct qcom_scmi_vendor_ops *ops;
static struct scmi_device *sdev;

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

static int mpam_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;

	sdev = get_qcom_scmi_device();
	if (IS_ERR(sdev)) {
		ret = PTR_ERR(sdev);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Error getting scmi_dev ret=%d\n", ret);
		return ret;
	}
	ops = sdev->handle->devm_protocol_get(sdev, QCOM_SCMI_VENDOR_PROTOCOL, &ph);
	if (IS_ERR(ops)) {
		ret = PTR_ERR(ops);
		ops = NULL;
		dev_err(dev, "Error getting vendor protocol ops: %d\n", ret);
		return ret;
	}

	return ret;
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
	.probe = mpam_dev_probe,
};

module_platform_driver(qcom_mpam_driver);

MODULE_SOFTDEP("pre: qcom_scmi_client");
MODULE_DESCRIPTION("QCOM MPAM driver");
MODULE_LICENSE("GPL");
