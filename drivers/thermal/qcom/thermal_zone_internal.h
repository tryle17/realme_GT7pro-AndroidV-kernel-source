/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022-2023, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __QTI_THERMAL_ZONE_INTERNAL_H
#define __QTI_THERMAL_ZONE_INTERNAL_H

#include <linux/thermal.h>
#include "../thermal_core.h"

/* Generic helpers for thermal zone -> change_mode ops */
static inline __maybe_unused int qti_tz_change_mode(struct thermal_zone_device *tz,
		enum thermal_device_mode mode)
{
	struct thermal_instance *instance;

	if (!tz)
		return 0;

	tz->passive = 0;
	tz->temperature = THERMAL_TEMP_INVALID;
	tz->prev_low_trip = -INT_MAX;
	tz->prev_high_trip = INT_MAX;
	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		instance->initialized = false;
		if (mode == THERMAL_DEVICE_DISABLED) {
			instance->target = THERMAL_NO_TARGET;
			instance->cdev->updated = false;
			thermal_cdev_update(instance->cdev);
		}
	}

	return 0;
}

#endif  // __QTI_THERMAL_ZONE_INTERNAL_H
