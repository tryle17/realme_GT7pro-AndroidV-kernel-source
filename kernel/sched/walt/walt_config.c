// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "walt.h"
#include "trace.h"
#include <soc/qcom/socinfo.h>

unsigned long __read_mostly soc_flags;
unsigned int trailblazer_floor_freq[MAX_CLUSTERS];
cpumask_t asym_cap_sibling_cpus;
cpumask_t pipeline_sync_cpus;
int oscillate_period_ns;
int soc_sched_lib_name_capacity;
#define PIPELINE_BUSY_THRESH_8MS_WINDOW 7
#define PIPELINE_BUSY_THRESH_12MS_WINDOW 11
#define PIPELINE_BUSY_THRESH_16MS_WINDOW 15

void walt_config(void)
{
	int i, j, cpu;
	const char *name = socinfo_get_id_string();

	/* —— 全局默认参数（节能优先，略牺牲性能） —— */
	sysctl_sched_group_upmigrate_pct         = 115;                          // 提高上迁门限，减少轻载唤醒大核
	sysctl_sched_group_downmigrate_pct       =  90;                          // 降低下迁门限，更早回小核
	sysctl_sched_task_unfilter_period        = 150000000;                    // 延长 unfilter 周期，减少频繁触发
	sysctl_sched_window_stats_policy         = WINDOW_STATS_AVG;             // 平滑窗口统计，降低波动
	sysctl_sched_ravg_window_nr_ticks        = (HZ / (NR_WINDOWS_PER_SEC/2)); // 加长统计窗口
	sched_load_granule                        = (DEFAULT_SCHED_RAVG_WINDOW * 2) / NUM_LOAD_INDICES;
	sysctl_sched_coloc_busy_hyst_enable_cpus =   64;                           // 减少共置检测 CPU 数
	sysctl_sched_util_busy_hyst_enable_cpus  =  128;                          // 减少 util 忙检测 CPU 数
	sysctl_sched_coloc_busy_hyst_max_ms      = 3000;                          // 缩短共置持续时长
	sched_ravg_window                        = DEFAULT_SCHED_RAVG_WINDOW * 2; // 双倍平滑窗口
	sysctl_input_boost_ms                    =   30;                          // 缩短输入触发的 boost 时长
	sysctl_sched_min_task_util_for_boost     =   60;                          // 仅中高 util 任务才 boost
	sysctl_sched_min_task_util_for_uclamp    =   60;                          // 仅中高 util 任务才 uclamp
	sysctl_sched_min_task_util_for_colocation=   40;                          // 提高共置门限，减少共置
	sysctl_sched_many_wakeup_threshold       = WALT_MANY_WAKEUP_DEFAULT * 2/3;
	sysctl_walt_rtg_cfs_boost_prio           =   99; /* disabled by default */
	sysctl_sched_sync_hint_enable            =    1;
	sysctl_panic_on_walt_bug                 = walt_debug_initial_values();
	sysctl_sched_skip_sp_newly_idle_lb       =    1;
	sysctl_sched_hyst_min_coloc_ns           = 80000000;
	sysctl_sched_idle_enough                 = SCHED_IDLE_ENOUGH_DEFAULT * 2; // 放宽 idle 判定，减少唤醒
	sysctl_sched_cluster_util_thres_pct      = SCHED_CLUSTER_UTIL_THRES_PCT_DEFAULT;
	sysctl_em_inflate_pct                    =   80;                          // 降低 EM 放大比例
	sysctl_em_inflate_thres                  =  512;                          // 降低 EM 触发阈值
	sysctl_max_freq_partial_halt             = FREQ_QOS_MAX_DEFAULT_VALUE / 2; // 容许更深 partial-halt
	asym_cap_sibling_cpus                     = CPU_MASK_NONE;
	pipeline_sync_cpus                        = CPU_MASK_NONE;

	for_each_possible_cpu(cpu) {
		for (i = 0; i < LEGACY_SMART_FREQ; i++)
			smart_freq_legacy_reason_hyst_ms[i][cpu] =
				(i ? 4 : 0);
	}

	for (i = 0; i < MAX_MARGIN_LEVELS; i++) {
		sysctl_sched_capacity_margin_up_pct[i] = 90;  // 略增 margin 减少 ups
		sysctl_sched_capacity_margin_dn_pct[i] = 80;  // 略增 margin 提前 downs
		sysctl_sched_early_up[i]               = 1077;
		sysctl_sched_early_down[i]             = 1204;
	}

	for (i = 0; i < WALT_NR_CPUS; i++) {
		sysctl_sched_coloc_busy_hyst_cpu[i]         = 39000000;
		sysctl_sched_coloc_busy_hyst_cpu_busy_pct[i]= 10;
		sysctl_sched_util_busy_hyst_cpu[i]          = 5000000;
		sysctl_sched_util_busy_hyst_cpu_util[i]     = 15;
		sysctl_input_boost_freq[i]                  = 0;
	}

	for (i = 0; i < MAX_CLUSTERS; i++) {
		sysctl_freq_cap[i]                          = FREQ_QOS_MAX_DEFAULT_VALUE;
		high_perf_cluster_freq_cap[i]               = FREQ_QOS_MAX_DEFAULT_VALUE;
		sysctl_sched_idle_enough_clust[i]           = SCHED_IDLE_ENOUGH_DEFAULT * 2;
		sysctl_sched_cluster_util_thres_pct_clust[i]= SCHED_CLUSTER_UTIL_THRES_PCT_DEFAULT;
		trailblazer_floor_freq[i]                   = 0;
		for (j = 0; j < MAX_CLUSTERS; j++) {
			load_sync_util_thres[i][j] = 0;
			load_sync_low_pct[i][j]   = 0;
			load_sync_high_pct[i][j]  = 0;
		}
	}

	for (i = 0; i < MAX_FREQ_CAP; i++) {
		for (j = 0; j < MAX_CLUSTERS; j++)
			freq_cap[i][j] = FREQ_QOS_MAX_DEFAULT_VALUE;
	}

	sysctl_sched_lrpb_active_ms[0] = PIPELINE_BUSY_THRESH_8MS_WINDOW;
	sysctl_sched_lrpb_active_ms[1] = PIPELINE_BUSY_THRESH_12MS_WINDOW;
	sysctl_sched_lrpb_active_ms[2] = PIPELINE_BUSY_THRESH_16MS_WINDOW;

	soc_feat_set(SOC_ENABLE_CONSERVATIVE_BOOST_TOPAPP_BIT);
	soc_feat_set(SOC_ENABLE_CONSERVATIVE_BOOST_FG_BIT);
	soc_feat_set(SOC_ENABLE_UCLAMP_BOOSTED_BIT);
	soc_feat_set(SOC_ENABLE_PER_TASK_BOOST_ON_MID_BIT);
	soc_feat_set(SOC_ENABLE_COLOCATION_PLACEMENT_BOOST_BIT);
	soc_feat_set(SOC_ENABLE_PIPELINE_SWAPPING_BIT);

	/* —— 平台差异化 —— */
	if (!name)
		return;

	if (!strcmp(name, "SUN")) {
		/* SUN 平台专属优化（见前文） */
		sysctl_sched_suppress_region2          = 1;
		sysctl_sched_group_upmigrate_pct       = 120;
		sysctl_sched_group_downmigrate_pct     = 85;
		sysctl_sched_min_task_util_for_boost   = 65;
		sysctl_sched_min_task_util_for_uclamp  = 65;
		sysctl_input_boost_ms                  = 20;
		soc_feat_unset(SOC_ENABLE_CONSERVATIVE_BOOST_TOPAPP_BIT);
		soc_feat_unset(SOC_ENABLE_UCLAMP_BOOSTED_BIT);
		soc_feat_unset(SOC_ENABLE_PER_TASK_BOOST_ON_MID_BIT);
		trailblazer_floor_freq[0]              = 768000;
		sysctl_sched_min_task_util_for_colocation = 45;
		sysctl_sched_coloc_busy_hyst_max_ms       = 2000;
		cpumask_clear(&pipeline_sync_cpus);
	} else if (!strcmp(name, "PINEAPPLE")) {
		soc_feat_set(SOC_ENABLE_SILVER_RT_SPREAD_BIT);
		soc_feat_set(SOC_ENABLE_BOOST_TO_NEXT_CLUSTER_BIT);

		/* T + G */
		cpumask_or(&asym_cap_sibling_cpus,
			&asym_cap_sibling_cpus, &cpu_array[0][1]);
		cpumask_or(&asym_cap_sibling_cpus,
			&asym_cap_sibling_cpus, &cpu_array[0][2]);

		/*
		 * Treat Golds and Primes as candidates for load sync under pipeline usecase.
		 * However, it is possible that a single CPU is not present. As prime is the
		 * only cluster with only one CPU, guard this setting by ensuring 4 clusters
		 * are present.
		 */
		if (num_sched_clusters == 4) {
			cpumask_or(&pipeline_sync_cpus,
				&pipeline_sync_cpus, &cpu_array[0][2]);
			cpumask_or(&pipeline_sync_cpus,
				&pipeline_sync_cpus, &cpu_array[0][3]);
		}

		sysctl_cluster23_load_sync[0]	= 350;
		sysctl_cluster23_load_sync[1]	= 100;
		sysctl_cluster23_load_sync[2]	= 100;
		sysctl_cluster32_load_sync[0]	= 512;
		sysctl_cluster32_load_sync[1]	= 90;
		sysctl_cluster32_load_sync[2]	= 90;
		load_sync_util_thres[2][3]	= sysctl_cluster23_load_sync[0];
		load_sync_low_pct[2][3]		= sysctl_cluster23_load_sync[1];
		load_sync_high_pct[2][3]	= sysctl_cluster23_load_sync[2];
		load_sync_util_thres[3][2]	= sysctl_cluster32_load_sync[0];
		load_sync_low_pct[3][2]		= sysctl_cluster32_load_sync[1];
		load_sync_high_pct[3][2]	= sysctl_cluster32_load_sync[2];
	}

	smart_freq_init(name);
}
