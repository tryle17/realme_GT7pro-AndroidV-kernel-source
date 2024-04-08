// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/thermal.h>
#include <trace/hooks/thermal.h>

static void disable_cdev_stats(void *unused,
		struct thermal_cooling_device *cdev, bool *disable)
{
	*disable = true;
}

static int __init qcom_thermal_vendor_hook_driver_init(void)
{
	int ret;

	ret = register_trace_android_vh_disable_thermal_cooling_stats(
			disable_cdev_stats, NULL);
	if (ret) {
		pr_err("Failed to register disable thermal cdev stats hooks\n");
		return ret;
	}

	return 0;
}

static void __exit qcom_thermal_vendor_hook_driver_exit(void)
{
	unregister_trace_android_vh_disable_thermal_cooling_stats(
			disable_cdev_stats, NULL);
}

#if IS_MODULE(CONFIG_QTI_THERMAL_VENDOR_HOOK)
module_init(qcom_thermal_vendor_hook_driver_init);
#else
pure_initcall(qcom_thermal_vendor_hook_driver_init);
#endif
module_exit(qcom_thermal_vendor_hook_driver_exit);

MODULE_DESCRIPTION("QCOM Thermal Vendor Hooks Driver");
MODULE_LICENSE("GPL");
