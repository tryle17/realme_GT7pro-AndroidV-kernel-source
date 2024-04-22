/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _QCOM_MPAM_H
#define _QCOM_MPAM_H

#include <linux/types.h>

#define SYS_MPAM0_EL1		sys_reg(3, 0, 10, 5, 1)
#define SYS_MPAM1_EL1		sys_reg(3, 0, 10, 5, 0)

#define PARTID_BITS			16
#define PARTID_I_SHIFT		0
#define PARTID_D_SHIFT		(PARTID_I_SHIFT + PARTID_BITS)

#define PARTID_DEFAULT		0
#define PARTID_RESERVED		16
#define PARTID_MAX			64
#define PARTID_AVAILABLE	(PARTID_MAX - PARTID_RESERVED)

#define MSMON_CFG_MON_SEL	0x0800
#define MSMON_CFG_CSU_FLT	0x0810
#define MSMON_CFG_CSU_CTL	0x0818
#define MSMON_CFG_MBWU_FLT	0x0820
#define MSMON_CFG_MBWU_CTL	0x0828
#define MSMON_CSU			0x0840
#define MSMON_MBWU			0x0860
#define MSMON_MBWU_L		0x0880

#define MAX_MONITOR_INSTANCES			16 /* Reserved 12:15 */
#define MAX_MONITOR_INSTANCES_SHARED	12
#define MONITOR_MAX		MAX_MONITOR_INSTANCES_SHARED

#define MPAM_MAX_RETRY		5000

enum msc_id {
	MSC_0 = 0,
	MSC_1,
	MSC_MAX,
};

/* Supported Mode */
enum mpam_config_mode {
	SET_CACHE_CAPACITY = 0,
	SET_CPBM,
	SET_DSPRI,
	SET_CACHE_CAPICITY_AND_CPBM,
	SET_CACHE_CAPACITY_AND_CPBM_AND_DSPRI,
	MAX_MODE
};

/* Monitor type */
enum mpam_monitor_type {
	MPAM_TYPE_MBW_MONITOR = 1,
	MPAM_TYPE_CSU_MONITOR = 2
};

/* PARAM_SET_CACHE_PARTITION */
struct mpam_set_cache_partition {
	uint32_t part_id;
	uint32_t cache_capacity;
	uint32_t cpbm_mask;
	uint32_t dspri;
	/*
	 * [0:8] - mode
	 * --[0x00] : set cache_capicity
	 * --[0x01] : set cpbm
	 * --[0x02] : set dspri
	 * --[0x03] : set cache_capicity & cpbm
	 * --[0x04] : set cache_capicity & cpbm & dspri
	 * [9:63] - Reserved
	 */
	uint64_t mpam_config_ctrl;
	uint32_t msc_id;
} __packed;

/* Part ID and monitor Parameters */
struct mpam_monitor_configuration {
	uint32_t msc_id;
	uint32_t part_id;
	uint32_t mon_instance;
	uint32_t mon_type;
	/* Filter and control bits */
	uint64_t mpam_config_ctrl;
} __packed;

/* PARAM_GET_VERSION */
struct mpam_ver_ret {
	uint32_t version;
};

/* PARAM_GET_CACHE_PARTITION */
struct mpam_read_cache_portion {
	uint32_t msc_id;
	uint32_t part_id;
} __packed;

struct mpam_slice_val {
	uint32_t cpbm;
	uint32_t capacity;
	uint32_t dspri;
} __packed;

struct monitors_value {
	uint32_t capture_in_progress;
	uint32_t msc_id;
	uint32_t csu_mon_enable_map[MAX_MONITOR_INSTANCES];
	uint32_t csu_mon_value[MAX_MONITOR_INSTANCES];
	uint32_t mbw_mon_enable_map[MAX_MONITOR_INSTANCES];
	uint64_t mbw_mon_value[MAX_MONITOR_INSTANCES];
	uint64_t last_capture_time;
} __packed;

#if IS_ENABLED(CONFIG_QTI_MPAM)
int qcom_mpam_set_cache_portion(struct mpam_set_cache_partition *msg);
int qcom_mpam_get_version(struct mpam_ver_ret *ver);
int qcom_mpam_get_cache_partition(struct mpam_read_cache_portion *param,
						struct mpam_slice_val *val);
int qcom_mpam_config_monitor(struct mpam_monitor_configuration *param);
#else
static inline int qcom_mpam_set_cache_portion(struct mpam_set_cache_partition *msg)
{
	return 0;
}

static inline int qcom_mpam_get_version(struct mpam_ver_ret *ver)
{
	return 0;
}

static inline int qcom_mpam_get_cache_partition(struct mpam_read_cache_portion *param,
						struct mpam_slice_val *val)
{
	return 0;
}

static inline int qcom_mpam_config_monitor(struct mpam_monitor_configuration *param)
{
	return 0;
}
#endif

#endif /* _QCOM_MPAM_H */
