// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/pm_runtime.h>

#include <dt-bindings/clock/qcom,camcc-pineapple.h>

#include "clk-alpha-pll.h"
#include "clk-branch.h"
#include "clk-pll.h"
#include "clk-rcg.h"
#include "clk-regmap.h"
#include "clk-regmap-divider.h"
#include "clk-regmap-mux.h"
#include "common.h"
#include "reset.h"
#include "vdd-level.h"

static DEFINE_VDD_REGULATORS(vdd_mm, VDD_NOMINAL + 1, 1, vdd_corner);
static DEFINE_VDD_REGULATORS(vdd_mxa, VDD_LOW + 1, 1, vdd_corner);
static DEFINE_VDD_REGULATORS(vdd_mxc, VDD_HIGH + 1, 1, vdd_corner);

static struct clk_vdd_class *cam_cc_pineapple_regulators[] = {
	&vdd_mm,
	&vdd_mxa,
	&vdd_mxc,
};

static struct clk_vdd_class *cam_cc_pineapple_regulators_1[] = {
	&vdd_mm,
	&vdd_mxc,
};

static struct clk_crm cam_crm = {
	.name = "cam_crm",
	.regs = {
		.cfg_rcgr = 0x110,
		.l_val = 0x138,
		.curr_perf = 0xc,
	},
	.offsets = {
		.vcd = 0x200,
		.level = 0x4,
	},
};

enum {
	P_BI_TCXO,
	P_CAM_CC_PLL0_OUT_EVEN,
	P_CAM_CC_PLL0_OUT_MAIN,
	P_CAM_CC_PLL0_OUT_ODD,
	P_CAM_CC_PLL10_OUT_EVEN,
	P_CAM_CC_PLL1_OUT_EVEN,
	P_CAM_CC_PLL2_OUT_EVEN,
	P_CAM_CC_PLL2_OUT_MAIN,
	P_CAM_CC_PLL3_OUT_EVEN,
	P_CAM_CC_PLL4_OUT_EVEN,
	P_CAM_CC_PLL5_OUT_EVEN,
	P_CAM_CC_PLL6_OUT_EVEN,
	P_CAM_CC_PLL7_OUT_EVEN,
	P_CAM_CC_PLL8_OUT_EVEN,
	P_CAM_CC_PLL9_OUT_EVEN,
	P_CAM_CC_PLL9_OUT_ODD,
	P_SLEEP_CLK,
};

static const struct pll_vco lucid_ole_vco[] = {
	{ 249600000, 2100000000, 0 },
};

static const struct pll_vco rivian_ole_vco[] = {
	{ 777000000, 1285000000, 0 },
};

static const struct alpha_pll_config cam_cc_pll0_config = {
	.l = 0x3E,
	.cal_l = 0x44,
	.cal_l_ringosc = 0x44,
	.alpha = 0x8000,
	.config_ctl_val = 0x20485699,
	.config_ctl_hi_val = 0x00182261,
	.config_ctl_hi1_val = 0x82AA299C,
	.test_ctl_val = 0x00000000,
	.test_ctl_hi_val = 0x00000003,
	.test_ctl_hi1_val = 0x00009000,
	.test_ctl_hi2_val = 0x00000034,
	.user_ctl_val = 0x00008400,
	.user_ctl_hi_val = 0x00000005,
};

static struct clk_alpha_pll cam_cc_pll0 = {
	.offset = 0x0,
	.vco_table = lucid_ole_vco,
	.num_vco = ARRAY_SIZE(lucid_ole_vco),
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.flags = ENABLE_IN_PREPARE,
	.clkr = {
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_pll0",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "bi_tcxo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_lucid_ole_ops,
		},
		.vdd_data = {
			.vdd_class = &vdd_mxc,
			.num_rate_max = VDD_NUM,
			.rate_max = (unsigned long[VDD_NUM]) {
				[VDD_LOWER_D1] = 615000000,
				[VDD_LOW] = 1100000000,
				[VDD_LOW_L1] = 1600000000,
				[VDD_NOMINAL] = 2000000000,
				[VDD_HIGH_L1] = 2100000000},
		},
	},
};

static const struct clk_div_table post_div_table_cam_cc_pll0_out_even[] = {
	{ 0x1, 2 },
	{ }
};

static struct clk_alpha_pll_postdiv cam_cc_pll0_out_even = {
	.offset = 0x0,
	.post_div_shift = 10,
	.post_div_table = post_div_table_cam_cc_pll0_out_even,
	.num_post_div = ARRAY_SIZE(post_div_table_cam_cc_pll0_out_even),
	.width = 4,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_pll0_out_even",
		.parent_hws = (const struct clk_hw*[]){
			&cam_cc_pll0.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_alpha_pll_postdiv_lucid_ole_ops,
	},
};

static const struct clk_div_table post_div_table_cam_cc_pll0_out_odd[] = {
	{ 0x2, 3 },
	{ }
};

static struct clk_alpha_pll_postdiv cam_cc_pll0_out_odd = {
	.offset = 0x0,
	.post_div_shift = 14,
	.post_div_table = post_div_table_cam_cc_pll0_out_odd,
	.num_post_div = ARRAY_SIZE(post_div_table_cam_cc_pll0_out_odd),
	.width = 4,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_pll0_out_odd",
		.parent_hws = (const struct clk_hw*[]){
			&cam_cc_pll0.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_alpha_pll_postdiv_lucid_ole_ops,
	},
};

static const struct alpha_pll_config cam_cc_pll1_config = {
	.l = 0x2F,
	.cal_l = 0x44,
	.cal_l_ringosc = 0x44,
	.alpha = 0x6555,
	.config_ctl_val = 0x20485699,
	.config_ctl_hi_val = 0x00182261,
	.config_ctl_hi1_val = 0x82AA299C,
	.test_ctl_val = 0x00000000,
	.test_ctl_hi_val = 0x00000003,
	.test_ctl_hi1_val = 0x00009000,
	.test_ctl_hi2_val = 0x00000034,
	.user_ctl_val = 0x00000400,
	.user_ctl_hi_val = 0x00000005,
};

static const struct alpha_pll_config cam_cc_pll1_config_pineapple_v2 = {
	.l = 0x31,
	.cal_l = 0x44,
	.cal_l_ringosc = 0x44,
	.alpha = 0x7AAA,
	.config_ctl_val = 0x20485699,
	.config_ctl_hi_val = 0x00182261,
	.config_ctl_hi1_val = 0x82AA299C,
	.test_ctl_val = 0x00000000,
	.test_ctl_hi_val = 0x00000000,
	.test_ctl_hi1_val = 0x00008000,
	.test_ctl_hi2_val = 0x00000032,
	.user_ctl_val = 0x00000401,
	.user_ctl_hi_val = 0x00000805,
};

static struct clk_alpha_pll cam_cc_pll1 = {
	.offset = 0x1000,
	.vco_table = lucid_ole_vco,
	.num_vco = ARRAY_SIZE(lucid_ole_vco),
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr = {
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_pll1",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "bi_tcxo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_lucid_ole_ops,
		},
		.vdd_data = {
			.vdd_class = &vdd_mxc,
			.num_rate_max = VDD_NUM,
			.rate_max = (unsigned long[VDD_NUM]) {
				[VDD_LOWER_D1] = 615000000,
				[VDD_LOW] = 1100000000,
				[VDD_LOW_L1] = 1600000000,
				[VDD_NOMINAL] = 2000000000,
				[VDD_HIGH_L1] = 2100000000},
		},
	},
};

static const struct clk_div_table post_div_table_cam_cc_pll1_out_even[] = {
	{ 0x1, 2 },
	{ }
};

static struct clk_alpha_pll_postdiv cam_cc_pll1_out_even = {
	.offset = 0x1000,
	.post_div_shift = 10,
	.post_div_table = post_div_table_cam_cc_pll1_out_even,
	.num_post_div = ARRAY_SIZE(post_div_table_cam_cc_pll1_out_even),
	.width = 4,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_pll1_out_even",
		.parent_hws = (const struct clk_hw*[]){
			&cam_cc_pll1.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_alpha_pll_postdiv_lucid_ole_ops,
	},
};

static const struct alpha_pll_config cam_cc_pll10_config = {
	.l = 0x30,
	.cal_l = 0x44,
	.cal_l_ringosc = 0x44,
	.alpha = 0x8AAA,
	.config_ctl_val = 0x20485699,
	.config_ctl_hi_val = 0x00182261,
	.config_ctl_hi1_val = 0x82AA299C,
	.test_ctl_val = 0x00000000,
	.test_ctl_hi_val = 0x00000003,
	.test_ctl_hi1_val = 0x00009000,
	.test_ctl_hi2_val = 0x00000034,
	.user_ctl_val = 0x00000400,
	.user_ctl_hi_val = 0x00000005,
};

static struct clk_alpha_pll cam_cc_pll10 = {
	.offset = 0xa000,
	.vco_table = lucid_ole_vco,
	.num_vco = ARRAY_SIZE(lucid_ole_vco),
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr = {
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_pll10",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "bi_tcxo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_crm_lucid_ole_ops,
		},
		.vdd_data = {
			.vdd_class = &vdd_mxc,
			.num_rate_max = VDD_NUM,
			.rate_max = (unsigned long[VDD_NUM]) {
				[VDD_LOWER_D1] = 615000000,
				[VDD_LOW] = 1100000000,
				[VDD_LOW_L1] = 1600000000,
				[VDD_NOMINAL] = 2000000000,
				[VDD_HIGH_L1] = 2100000000},
		},
	},
};

static const struct clk_div_table post_div_table_cam_cc_pll10_out_even[] = {
	{ 0x1, 2 },
	{ }
};

static struct clk_alpha_pll_postdiv cam_cc_pll10_out_even = {
	.offset = 0xa000,
	.post_div_shift = 10,
	.post_div_table = post_div_table_cam_cc_pll10_out_even,
	.num_post_div = ARRAY_SIZE(post_div_table_cam_cc_pll10_out_even),
	.width = 4,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_pll10_out_even",
		.parent_hws = (const struct clk_hw*[]){
			&cam_cc_pll10.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_alpha_pll_crm_postdiv_lucid_ole_ops,
	},
};

static const struct alpha_pll_config cam_cc_pll2_config = {
	.l = 0x32,
	.cal_l = 0x32,
	.alpha = 0x0,
	.config_ctl_val = 0x10000030,
	.config_ctl_hi_val = 0x80890263,
	.config_ctl_hi1_val = 0x00000217,
	.user_ctl_val = 0x00000001,
	.user_ctl_hi_val = 0x00000000,
};

static struct clk_alpha_pll cam_cc_pll2 = {
	.offset = 0x2000,
	.vco_table = rivian_ole_vco,
	.num_vco = ARRAY_SIZE(rivian_ole_vco),
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_RIVIAN_OLE],
	.clkr = {
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_pll2",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "bi_tcxo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_rivian_ole_ops,
		},
		.vdd_data = {
			.vdd_class = &vdd_mxa,
			.num_rate_max = VDD_NUM,
			.rate_max = (unsigned long[VDD_NUM]) {
				[VDD_LOW] = 1285000000},
		},
	},
};

static const struct alpha_pll_config cam_cc_pll3_config = {
	.l = 0x30,
	.cal_l = 0x44,
	.cal_l_ringosc = 0x44,
	.alpha = 0x8AAA,
	.config_ctl_val = 0x20485699,
	.config_ctl_hi_val = 0x00182261,
	.config_ctl_hi1_val = 0x82AA299C,
	.test_ctl_val = 0x00000000,
	.test_ctl_hi_val = 0x00000003,
	.test_ctl_hi1_val = 0x00009000,
	.test_ctl_hi2_val = 0x00000034,
	.user_ctl_val = 0x00000400,
	.user_ctl_hi_val = 0x00000005,
};

static struct clk_alpha_pll cam_cc_pll3 = {
	.offset = 0x3000,
	.vco_table = lucid_ole_vco,
	.num_vco = ARRAY_SIZE(lucid_ole_vco),
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr = {
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_pll3",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "bi_tcxo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_crm_lucid_ole_ops,
		},
		.vdd_data = {
			.vdd_class = &vdd_mxc,
			.num_rate_max = VDD_NUM,
			.rate_max = (unsigned long[VDD_NUM]) {
				[VDD_LOWER_D1] = 615000000,
				[VDD_LOW] = 1100000000,
				[VDD_LOW_L1] = 1600000000,
				[VDD_NOMINAL] = 2000000000,
				[VDD_HIGH_L1] = 2100000000},
		},
	},
};

static const struct clk_div_table post_div_table_cam_cc_pll3_out_even[] = {
	{ 0x1, 2 },
	{ }
};

static struct clk_alpha_pll_postdiv cam_cc_pll3_out_even = {
	.offset = 0x3000,
	.post_div_shift = 10,
	.post_div_table = post_div_table_cam_cc_pll3_out_even,
	.num_post_div = ARRAY_SIZE(post_div_table_cam_cc_pll3_out_even),
	.width = 4,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_pll3_out_even",
		.parent_hws = (const struct clk_hw*[]){
			&cam_cc_pll3.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_alpha_pll_crm_postdiv_lucid_ole_ops,
	},
};

static const struct alpha_pll_config cam_cc_pll4_config = {
	.l = 0x30,
	.cal_l = 0x44,
	.cal_l_ringosc = 0x44,
	.alpha = 0x8AAA,
	.config_ctl_val = 0x20485699,
	.config_ctl_hi_val = 0x00182261,
	.config_ctl_hi1_val = 0x82AA299C,
	.test_ctl_val = 0x00000000,
	.test_ctl_hi_val = 0x00000003,
	.test_ctl_hi1_val = 0x00009000,
	.test_ctl_hi2_val = 0x00000034,
	.user_ctl_val = 0x00000400,
	.user_ctl_hi_val = 0x00000005,
};

static struct clk_alpha_pll cam_cc_pll4 = {
	.offset = 0x4000,
	.vco_table = lucid_ole_vco,
	.num_vco = ARRAY_SIZE(lucid_ole_vco),
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr = {
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_pll4",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "bi_tcxo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_crm_lucid_ole_ops,
		},
		.vdd_data = {
			.vdd_class = &vdd_mxc,
			.num_rate_max = VDD_NUM,
			.rate_max = (unsigned long[VDD_NUM]) {
				[VDD_LOWER_D1] = 615000000,
				[VDD_LOW] = 1100000000,
				[VDD_LOW_L1] = 1600000000,
				[VDD_NOMINAL] = 2000000000,
				[VDD_HIGH_L1] = 2100000000},
		},
	},
};

static const struct clk_div_table post_div_table_cam_cc_pll4_out_even[] = {
	{ 0x1, 2 },
	{ }
};

static struct clk_alpha_pll_postdiv cam_cc_pll4_out_even = {
	.offset = 0x4000,
	.post_div_shift = 10,
	.post_div_table = post_div_table_cam_cc_pll4_out_even,
	.num_post_div = ARRAY_SIZE(post_div_table_cam_cc_pll4_out_even),
	.width = 4,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_pll4_out_even",
		.parent_hws = (const struct clk_hw*[]){
			&cam_cc_pll4.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_alpha_pll_crm_postdiv_lucid_ole_ops,
	},
};

static const struct alpha_pll_config cam_cc_pll5_config = {
	.l = 0x30,
	.cal_l = 0x44,
	.cal_l_ringosc = 0x44,
	.alpha = 0x8AAA,
	.config_ctl_val = 0x20485699,
	.config_ctl_hi_val = 0x00182261,
	.config_ctl_hi1_val = 0x82AA299C,
	.test_ctl_val = 0x00000000,
	.test_ctl_hi_val = 0x00000003,
	.test_ctl_hi1_val = 0x00009000,
	.test_ctl_hi2_val = 0x00000034,
	.user_ctl_val = 0x00000400,
	.user_ctl_hi_val = 0x00000005,
};

static struct clk_alpha_pll cam_cc_pll5 = {
	.offset = 0x5000,
	.vco_table = lucid_ole_vco,
	.num_vco = ARRAY_SIZE(lucid_ole_vco),
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr = {
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_pll5",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "bi_tcxo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_crm_lucid_ole_ops,
		},
		.vdd_data = {
			.vdd_class = &vdd_mxc,
			.num_rate_max = VDD_NUM,
			.rate_max = (unsigned long[VDD_NUM]) {
				[VDD_LOWER_D1] = 615000000,
				[VDD_LOW] = 1100000000,
				[VDD_LOW_L1] = 1600000000,
				[VDD_NOMINAL] = 2000000000,
				[VDD_HIGH_L1] = 2100000000},
		},
	},
};

static const struct clk_div_table post_div_table_cam_cc_pll5_out_even[] = {
	{ 0x1, 2 },
	{ }
};

static struct clk_alpha_pll_postdiv cam_cc_pll5_out_even = {
	.offset = 0x5000,
	.post_div_shift = 10,
	.post_div_table = post_div_table_cam_cc_pll5_out_even,
	.num_post_div = ARRAY_SIZE(post_div_table_cam_cc_pll5_out_even),
	.width = 4,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_pll5_out_even",
		.parent_hws = (const struct clk_hw*[]){
			&cam_cc_pll5.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_alpha_pll_crm_postdiv_lucid_ole_ops,
	},
};

static const struct alpha_pll_config cam_cc_pll6_config = {
	.l = 0x30,
	.cal_l = 0x44,
	.cal_l_ringosc = 0x44,
	.alpha = 0x8AAA,
	.config_ctl_val = 0x20485699,
	.config_ctl_hi_val = 0x00182261,
	.config_ctl_hi1_val = 0x82AA299C,
	.test_ctl_val = 0x00000000,
	.test_ctl_hi_val = 0x00000003,
	.test_ctl_hi1_val = 0x00009000,
	.test_ctl_hi2_val = 0x00000034,
	.user_ctl_val = 0x00000400,
	.user_ctl_hi_val = 0x00000005,
};

static struct clk_alpha_pll cam_cc_pll6 = {
	.offset = 0x6000,
	.vco_table = lucid_ole_vco,
	.num_vco = ARRAY_SIZE(lucid_ole_vco),
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr = {
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_pll6",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "bi_tcxo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_crm_lucid_ole_ops,
		},
		.vdd_data = {
			.vdd_class = &vdd_mxc,
			.num_rate_max = VDD_NUM,
			.rate_max = (unsigned long[VDD_NUM]) {
				[VDD_LOWER_D1] = 615000000,
				[VDD_LOW] = 1100000000,
				[VDD_LOW_L1] = 1600000000,
				[VDD_NOMINAL] = 2000000000,
				[VDD_HIGH_L1] = 2100000000},
		},
	},
};

static const struct clk_div_table post_div_table_cam_cc_pll6_out_even[] = {
	{ 0x1, 2 },
	{ }
};

static struct clk_alpha_pll_postdiv cam_cc_pll6_out_even = {
	.offset = 0x6000,
	.post_div_shift = 10,
	.post_div_table = post_div_table_cam_cc_pll6_out_even,
	.num_post_div = ARRAY_SIZE(post_div_table_cam_cc_pll6_out_even),
	.width = 4,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_pll6_out_even",
		.parent_hws = (const struct clk_hw*[]){
			&cam_cc_pll6.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_alpha_pll_crm_postdiv_lucid_ole_ops,
	},
};

static const struct alpha_pll_config cam_cc_pll7_config = {
	.l = 0x30,
	.cal_l = 0x44,
	.cal_l_ringosc = 0x44,
	.alpha = 0x8AAA,
	.config_ctl_val = 0x20485699,
	.config_ctl_hi_val = 0x00182261,
	.config_ctl_hi1_val = 0x82AA299C,
	.test_ctl_val = 0x00000000,
	.test_ctl_hi_val = 0x00000003,
	.test_ctl_hi1_val = 0x00009000,
	.test_ctl_hi2_val = 0x00000034,
	.user_ctl_val = 0x00000400,
	.user_ctl_hi_val = 0x00000005,
};

static struct clk_alpha_pll cam_cc_pll7 = {
	.offset = 0x7000,
	.vco_table = lucid_ole_vco,
	.num_vco = ARRAY_SIZE(lucid_ole_vco),
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr = {
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_pll7",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "bi_tcxo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_crm_lucid_ole_ops,
		},
		.vdd_data = {
			.vdd_class = &vdd_mxc,
			.num_rate_max = VDD_NUM,
			.rate_max = (unsigned long[VDD_NUM]) {
				[VDD_LOWER_D1] = 615000000,
				[VDD_LOW] = 1100000000,
				[VDD_LOW_L1] = 1600000000,
				[VDD_NOMINAL] = 2000000000,
				[VDD_HIGH_L1] = 2100000000},
		},
	},
};

static const struct clk_div_table post_div_table_cam_cc_pll7_out_even[] = {
	{ 0x1, 2 },
	{ }
};

static struct clk_alpha_pll_postdiv cam_cc_pll7_out_even = {
	.offset = 0x7000,
	.post_div_shift = 10,
	.post_div_table = post_div_table_cam_cc_pll7_out_even,
	.num_post_div = ARRAY_SIZE(post_div_table_cam_cc_pll7_out_even),
	.width = 4,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_pll7_out_even",
		.parent_hws = (const struct clk_hw*[]){
			&cam_cc_pll7.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_alpha_pll_crm_postdiv_lucid_ole_ops,
	},
};

static const struct alpha_pll_config cam_cc_pll8_config = {
	.l = 0x14,
	.cal_l = 0x44,
	.cal_l_ringosc = 0x44,
	.alpha = 0xD555,
	.config_ctl_val = 0x20485699,
	.config_ctl_hi_val = 0x00182261,
	.config_ctl_hi1_val = 0x82AA299C,
	.test_ctl_val = 0x00000000,
	.test_ctl_hi_val = 0x00000003,
	.test_ctl_hi1_val = 0x00009000,
	.test_ctl_hi2_val = 0x00000034,
	.user_ctl_val = 0x00000400,
	.user_ctl_hi_val = 0x00000005,
};

static struct clk_alpha_pll cam_cc_pll8 = {
	.offset = 0x8000,
	.vco_table = lucid_ole_vco,
	.num_vco = ARRAY_SIZE(lucid_ole_vco),
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr = {
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_pll8",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "bi_tcxo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_lucid_ole_ops,
		},
		.vdd_data = {
			.vdd_class = &vdd_mxc,
			.num_rate_max = VDD_NUM,
			.rate_max = (unsigned long[VDD_NUM]) {
				[VDD_LOWER_D1] = 615000000,
				[VDD_LOW] = 1100000000,
				[VDD_LOW_L1] = 1600000000,
				[VDD_NOMINAL] = 2000000000,
				[VDD_HIGH_L1] = 2100000000},
		},
	},
};

static const struct clk_div_table post_div_table_cam_cc_pll8_out_even[] = {
	{ 0x1, 2 },
	{ }
};

static struct clk_alpha_pll_postdiv cam_cc_pll8_out_even = {
	.offset = 0x8000,
	.post_div_shift = 10,
	.post_div_table = post_div_table_cam_cc_pll8_out_even,
	.num_post_div = ARRAY_SIZE(post_div_table_cam_cc_pll8_out_even),
	.width = 4,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_pll8_out_even",
		.parent_hws = (const struct clk_hw*[]){
			&cam_cc_pll8.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_alpha_pll_postdiv_lucid_ole_ops,
	},
};

static const struct alpha_pll_config cam_cc_pll9_config = {
	.l = 0x32,
	.cal_l = 0x44,
	.cal_l_ringosc = 0x44,
	.alpha = 0x0,
	.config_ctl_val = 0x20485699,
	.config_ctl_hi_val = 0x00182261,
	.config_ctl_hi1_val = 0x82AA299C,
	.test_ctl_val = 0x00000000,
	.test_ctl_hi_val = 0x00000003,
	.test_ctl_hi1_val = 0x00009000,
	.test_ctl_hi2_val = 0x00000034,
	.user_ctl_val = 0x00008400,
	.user_ctl_hi_val = 0x00000005,
};

static struct clk_alpha_pll cam_cc_pll9 = {
	.offset = 0x9000,
	.vco_table = lucid_ole_vco,
	.num_vco = ARRAY_SIZE(lucid_ole_vco),
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr = {
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_pll9",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "bi_tcxo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_lucid_ole_ops,
		},
		.vdd_data = {
			.vdd_class = &vdd_mxc,
			.num_rate_max = VDD_NUM,
			.rate_max = (unsigned long[VDD_NUM]) {
				[VDD_LOWER_D1] = 615000000,
				[VDD_LOW] = 1100000000,
				[VDD_LOW_L1] = 1600000000,
				[VDD_NOMINAL] = 2000000000,
				[VDD_HIGH_L1] = 2100000000},
		},
	},
};

static const struct clk_div_table post_div_table_cam_cc_pll9_out_even[] = {
	{ 0x1, 2 },
	{ }
};

static struct clk_alpha_pll_postdiv cam_cc_pll9_out_even = {
	.offset = 0x9000,
	.post_div_shift = 10,
	.post_div_table = post_div_table_cam_cc_pll9_out_even,
	.num_post_div = ARRAY_SIZE(post_div_table_cam_cc_pll9_out_even),
	.width = 4,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_pll9_out_even",
		.parent_hws = (const struct clk_hw*[]){
			&cam_cc_pll9.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_alpha_pll_postdiv_lucid_ole_ops,
	},
};

static const struct clk_div_table post_div_table_cam_cc_pll9_out_odd[] = {
	{ 0x2, 3 },
	{ }
};

static struct clk_alpha_pll_postdiv cam_cc_pll9_out_odd = {
	.offset = 0x9000,
	.post_div_shift = 14,
	.post_div_table = post_div_table_cam_cc_pll9_out_odd,
	.num_post_div = ARRAY_SIZE(post_div_table_cam_cc_pll9_out_odd),
	.width = 4,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_LUCID_OLE],
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_pll9_out_odd",
		.parent_hws = (const struct clk_hw*[]){
			&cam_cc_pll9.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_alpha_pll_postdiv_lucid_ole_ops,
	},
};

static const struct parent_map cam_cc_parent_map_0[] = {
	{ P_BI_TCXO, 0 },
	{ P_CAM_CC_PLL0_OUT_MAIN, 1 },
	{ P_CAM_CC_PLL0_OUT_EVEN, 2 },
	{ P_CAM_CC_PLL0_OUT_ODD, 3 },
	{ P_CAM_CC_PLL9_OUT_ODD, 4 },
	{ P_CAM_CC_PLL9_OUT_EVEN, 5 },
};

static const struct clk_parent_data cam_cc_parent_data_0[] = {
	{ .fw_name = "bi_tcxo" },
	{ .hw = &cam_cc_pll0.clkr.hw },
	{ .hw = &cam_cc_pll0_out_even.clkr.hw },
	{ .hw = &cam_cc_pll0_out_odd.clkr.hw },
	{ .hw = &cam_cc_pll9_out_odd.clkr.hw },
	{ .hw = &cam_cc_pll9_out_even.clkr.hw },
};

static const struct parent_map cam_cc_parent_map_1[] = {
	{ P_BI_TCXO, 0 },
	{ P_CAM_CC_PLL2_OUT_EVEN, 3 },
	{ P_CAM_CC_PLL2_OUT_MAIN, 5 },
};

static const struct clk_parent_data cam_cc_parent_data_1[] = {
	{ .fw_name = "bi_tcxo" },
	{ .hw = &cam_cc_pll2.clkr.hw },
	{ .hw = &cam_cc_pll2.clkr.hw },
};

static const struct parent_map cam_cc_parent_map_2[] = {
	{ P_BI_TCXO, 0 },
	{ P_CAM_CC_PLL8_OUT_EVEN, 6 },
};

static const struct clk_parent_data cam_cc_parent_data_2[] = {
	{ .fw_name = "bi_tcxo" },
	{ .hw = &cam_cc_pll8_out_even.clkr.hw },
};

static const struct parent_map cam_cc_parent_map_3[] = {
	{ P_BI_TCXO, 0 },
	{ P_CAM_CC_PLL3_OUT_EVEN, 6 },
};

static const struct clk_parent_data cam_cc_parent_data_3[] = {
	{ .fw_name = "bi_tcxo" },
	{ .hw = &cam_cc_pll3_out_even.clkr.hw },
};

static const struct parent_map cam_cc_parent_map_4[] = {
	{ P_BI_TCXO, 0 },
	{ P_CAM_CC_PLL4_OUT_EVEN, 6 },
};

static const struct clk_parent_data cam_cc_parent_data_4[] = {
	{ .fw_name = "bi_tcxo" },
	{ .hw = &cam_cc_pll4_out_even.clkr.hw },
};

static const struct parent_map cam_cc_parent_map_5[] = {
	{ P_BI_TCXO, 0 },
	{ P_CAM_CC_PLL5_OUT_EVEN, 6 },
};

static const struct clk_parent_data cam_cc_parent_data_5[] = {
	{ .fw_name = "bi_tcxo" },
	{ .hw = &cam_cc_pll5_out_even.clkr.hw },
};

static const struct parent_map cam_cc_parent_map_6[] = {
	{ P_BI_TCXO, 0 },
	{ P_CAM_CC_PLL1_OUT_EVEN, 4 },
};

static const struct clk_parent_data cam_cc_parent_data_6[] = {
	{ .fw_name = "bi_tcxo" },
	{ .hw = &cam_cc_pll1_out_even.clkr.hw },
};

static const struct parent_map cam_cc_parent_map_7[] = {
	{ P_BI_TCXO, 0 },
	{ P_CAM_CC_PLL6_OUT_EVEN, 6 },
};

static const struct clk_parent_data cam_cc_parent_data_7[] = {
	{ .fw_name = "bi_tcxo" },
	{ .hw = &cam_cc_pll6_out_even.clkr.hw },
};

static const struct parent_map cam_cc_parent_map_8[] = {
	{ P_BI_TCXO, 0 },
	{ P_CAM_CC_PLL7_OUT_EVEN, 6 },
};

static const struct clk_parent_data cam_cc_parent_data_8[] = {
	{ .fw_name = "bi_tcxo" },
	{ .hw = &cam_cc_pll7_out_even.clkr.hw },
};

static const struct parent_map cam_cc_parent_map_9[] = {
	{ P_BI_TCXO, 0 },
	{ P_CAM_CC_PLL10_OUT_EVEN, 6 },
};

static const struct clk_parent_data cam_cc_parent_data_9[] = {
	{ .fw_name = "bi_tcxo" },
	{ .hw = &cam_cc_pll10_out_even.clkr.hw },
};

static const struct parent_map cam_cc_parent_map_10[] = {
	{ P_SLEEP_CLK, 0 },
};

static const struct clk_parent_data cam_cc_parent_data_10[] = {
	{ .fw_name = "sleep_clk" },
};

static const struct parent_map cam_cc_parent_map_11[] = {
	{ P_BI_TCXO, 0 },
};

static const struct clk_parent_data cam_cc_parent_data_11_ao[] = {
	{ .fw_name = "bi_tcxo_ao" },
};

static const struct freq_tbl ftbl_cam_cc_bps_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	F(200000000, P_CAM_CC_PLL8_OUT_EVEN, 1, 0, 0),
	F(400000000, P_CAM_CC_PLL8_OUT_EVEN, 1, 0, 0),
	F(480000000, P_CAM_CC_PLL8_OUT_EVEN, 1, 0, 0),
	F(785000000, P_CAM_CC_PLL8_OUT_EVEN, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_bps_clk_src = {
	.cmd_rcgr = 0x10050,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_2,
	.freq_tbl = ftbl_cam_cc_bps_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_bps_clk_src",
		.parent_data = cam_cc_parent_data_2,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_2),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 200000000,
			[VDD_LOW] = 400000000,
			[VDD_LOW_L1] = 480000000,
			[VDD_NOMINAL] = 785000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_camnoc_axi_rt_clk_src[] = {
	F(300000000, P_CAM_CC_PLL0_OUT_EVEN, 2, 0, 0),
	F(400000000, P_CAM_CC_PLL0_OUT_ODD, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_camnoc_axi_rt_clk_src = {
	.cmd_rcgr = 0x1325c,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_camnoc_axi_rt_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr = {
		.crm = &cam_crm,
		.crm_vcd = 8,
		.crm_num_node = 2,
	},
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_camnoc_axi_rt_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_crmb_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 300000000,
			[VDD_LOW] = 400000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_cci_0_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	F(37500000, P_CAM_CC_PLL0_OUT_EVEN, 16, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_cci_0_clk_src = {
	.cmd_rcgr = 0x131cc,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_cci_0_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_cci_0_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mm,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 37500000},
	},
};

static struct clk_rcg2 cam_cc_cci_1_clk_src = {
	.cmd_rcgr = 0x131e8,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_cci_0_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_cci_1_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mm,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 37500000},
	},
};

static struct clk_rcg2 cam_cc_cci_2_clk_src = {
	.cmd_rcgr = 0x13204,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_cci_0_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_cci_2_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mm,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 37500000},
	},
};

static const struct freq_tbl ftbl_cam_cc_cphy_rx_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	F(400000000, P_CAM_CC_PLL0_OUT_MAIN, 3, 0, 0),
	F(480000000, P_CAM_CC_PLL0_OUT_MAIN, 2.5, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_cphy_rx_clk_src = {
	.cmd_rcgr = 0x1104c,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_cphy_rx_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr = {
		.crm = &cam_crm,
		.crm_vcd = 7,
	},
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_cphy_rx_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
		.ops = &clk_rcg2_crmc_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000,
			[VDD_LOW] = 480000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_cre_clk_src[] = {
	F(200000000, P_CAM_CC_PLL0_OUT_ODD, 2, 0, 0),
	F(400000000, P_CAM_CC_PLL0_OUT_ODD, 1, 0, 0),
	F(480000000, P_CAM_CC_PLL9_OUT_EVEN, 1, 0, 0),
	F(600000000, P_CAM_CC_PLL0_OUT_EVEN, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_cre_clk_src = {
	.cmd_rcgr = 0x13144,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_cre_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_cre_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mm,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 200000000,
			[VDD_LOW] = 400000000,
			[VDD_LOW_L1] = 480000000,
			[VDD_NOMINAL] = 600000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_csi0phytimer_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	F(400000000, P_CAM_CC_PLL0_OUT_MAIN, 3, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_csi0phytimer_clk_src = {
	.cmd_rcgr = 0x150e0,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_csi0phytimer_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_csi0phytimer_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxc,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000},
	},
};

static struct clk_rcg2 cam_cc_csi1phytimer_clk_src = {
	.cmd_rcgr = 0x15104,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_csi0phytimer_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_csi1phytimer_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxc,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000},
	},
};

static struct clk_rcg2 cam_cc_csi2phytimer_clk_src = {
	.cmd_rcgr = 0x15124,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_csi0phytimer_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_csi2phytimer_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxa,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000},
	},
};

static struct clk_rcg2 cam_cc_csi3phytimer_clk_src = {
	.cmd_rcgr = 0x15144,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_csi0phytimer_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_csi3phytimer_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxc,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000},
	},
};

static struct clk_rcg2 cam_cc_csi4phytimer_clk_src = {
	.cmd_rcgr = 0x15164,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_csi0phytimer_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_csi4phytimer_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxa,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000},
	},
};

static struct clk_rcg2 cam_cc_csi5phytimer_clk_src = {
	.cmd_rcgr = 0x15184,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_csi0phytimer_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_csi5phytimer_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxc,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000},
	},
};

static struct clk_rcg2 cam_cc_csi6phytimer_clk_src = {
	.cmd_rcgr = 0x151a4,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_csi0phytimer_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_csi6phytimer_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxc,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000},
	},
};

static struct clk_rcg2 cam_cc_csi7phytimer_clk_src = {
	.cmd_rcgr = 0x151c4,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_csi0phytimer_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_csi7phytimer_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxc,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_csid_clk_src[] = {
	F(400000000, P_CAM_CC_PLL0_OUT_MAIN, 3, 0, 0),
	F(480000000, P_CAM_CC_PLL0_OUT_MAIN, 2.5, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_csid_clk_src = {
	.cmd_rcgr = 0x13238,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_csid_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr = {
		.crm = &cam_crm,
		.crm_vcd = 6,
	},
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_csid_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
		.ops = &clk_rcg2_crmc_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000,
			[VDD_LOW] = 480000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_fast_ahb_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	F(300000000, P_CAM_CC_PLL0_OUT_EVEN, 2, 0, 0),
	F(400000000, P_CAM_CC_PLL0_OUT_MAIN, 3, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_fast_ahb_clk_src = {
	.cmd_rcgr = 0x10018,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_fast_ahb_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_fast_ahb_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 300000000,
			[VDD_NOMINAL] = 400000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_icp_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	F(400000000, P_CAM_CC_PLL0_OUT_ODD, 1, 0, 0),
	F(480000000, P_CAM_CC_PLL9_OUT_EVEN, 1, 0, 0),
	F(600000000, P_CAM_CC_PLL0_OUT_MAIN, 2, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_icp_clk_src = {
	.cmd_rcgr = 0x131a4,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_icp_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_icp_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000,
			[VDD_LOW] = 480000000,
			[VDD_LOW_L1] = 600000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_ife_0_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	F(466000000, P_CAM_CC_PLL3_OUT_EVEN, 1, 0, 0),
	F(594000000, P_CAM_CC_PLL3_OUT_EVEN, 1, 0, 0),
	F(675000000, P_CAM_CC_PLL3_OUT_EVEN, 1, 0, 0),
	F(785000000, P_CAM_CC_PLL3_OUT_EVEN, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_ife_0_clk_src = {
	.cmd_rcgr = 0x11018,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_3,
	.freq_tbl = ftbl_cam_cc_ife_0_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr = {
		.crm = &cam_crm,
		.crm_vcd = 0,
	},
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_ife_0_clk_src",
		.parent_data = cam_cc_parent_data_3,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_3),
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
		.ops = &clk_rcg2_crmc_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 466000000,
			[VDD_LOW] = 594000000,
			[VDD_LOW_L1] = 675000000,
			[VDD_NOMINAL] = 785000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_ife_1_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	F(466000000, P_CAM_CC_PLL4_OUT_EVEN, 1, 0, 0),
	F(594000000, P_CAM_CC_PLL4_OUT_EVEN, 1, 0, 0),
	F(675000000, P_CAM_CC_PLL4_OUT_EVEN, 1, 0, 0),
	F(785000000, P_CAM_CC_PLL4_OUT_EVEN, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_ife_1_clk_src = {
	.cmd_rcgr = 0x12018,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_4,
	.freq_tbl = ftbl_cam_cc_ife_1_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr = {
		.crm = &cam_crm,
		.crm_vcd = 1,
	},
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_ife_1_clk_src",
		.parent_data = cam_cc_parent_data_4,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_4),
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
		.ops = &clk_rcg2_crmc_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 466000000,
			[VDD_LOW] = 594000000,
			[VDD_LOW_L1] = 675000000,
			[VDD_NOMINAL] = 785000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_ife_2_clk_src[] = {
	F(466000000, P_CAM_CC_PLL5_OUT_EVEN, 1, 0, 0),
	F(594000000, P_CAM_CC_PLL5_OUT_EVEN, 1, 0, 0),
	F(675000000, P_CAM_CC_PLL5_OUT_EVEN, 1, 0, 0),
	F(785000000, P_CAM_CC_PLL5_OUT_EVEN, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_ife_2_clk_src = {
	.cmd_rcgr = 0x12068,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_5,
	.freq_tbl = ftbl_cam_cc_ife_2_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr = {
		.crm = &cam_crm,
		.crm_vcd = 2,
	},
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_ife_2_clk_src",
		.parent_data = cam_cc_parent_data_5,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_5),
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
		.ops = &clk_rcg2_crmc_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 466000000,
			[VDD_LOW] = 594000000,
			[VDD_LOW_L1] = 675000000,
			[VDD_NOMINAL] = 785000000},
	},
};

static struct clk_rcg2 cam_cc_ife_lite_clk_src = {
	.cmd_rcgr = 0x13000,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_csid_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_ife_lite_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000,
			[VDD_LOW] = 480000000},
	},
};

static struct clk_rcg2 cam_cc_ife_lite_csid_clk_src = {
	.cmd_rcgr = 0x13028,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_csid_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_ife_lite_csid_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 400000000,
			[VDD_LOW] = 480000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_ipe_nps_clk_src[] = {
	F(455000000, P_CAM_CC_PLL1_OUT_EVEN, 1, 0, 0),
	F(575000000, P_CAM_CC_PLL1_OUT_EVEN, 1, 0, 0),
	F(675000000, P_CAM_CC_PLL1_OUT_EVEN, 1, 0, 0),
	F(825000000, P_CAM_CC_PLL1_OUT_EVEN, 1, 0, 0),
	{ }
};

static const struct freq_tbl ftbl_cam_cc_ipe_nps_clk_src_pineapple_v2[] = {
	F(475000000, P_CAM_CC_PLL1_OUT_EVEN, 1, 0, 0),
	F(575000000, P_CAM_CC_PLL1_OUT_EVEN, 1, 0, 0),
	F(675000000, P_CAM_CC_PLL1_OUT_EVEN, 1, 0, 0),
	F(825000000, P_CAM_CC_PLL1_OUT_EVEN, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_ipe_nps_clk_src = {
	.cmd_rcgr = 0x10094,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_6,
	.freq_tbl = ftbl_cam_cc_ipe_nps_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_ipe_nps_clk_src",
		.parent_data = cam_cc_parent_data_6,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_6),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 455000000,
			[VDD_LOW] = 575000000,
			[VDD_LOW_L1] = 675000000,
			[VDD_NOMINAL] = 825000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_jpeg_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	F(200000000, P_CAM_CC_PLL0_OUT_ODD, 2, 0, 0),
	F(400000000, P_CAM_CC_PLL0_OUT_ODD, 1, 0, 0),
	F(480000000, P_CAM_CC_PLL9_OUT_EVEN, 1, 0, 0),
	F(600000000, P_CAM_CC_PLL0_OUT_EVEN, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_jpeg_clk_src = {
	.cmd_rcgr = 0x13168,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_jpeg_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_jpeg_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 200000000,
			[VDD_LOW] = 400000000,
			[VDD_LOW_L1] = 480000000,
			[VDD_NOMINAL] = 600000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_mclk0_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	F(24000000, P_CAM_CC_PLL2_OUT_EVEN, 10, 1, 4),
	F(68571429, P_CAM_CC_PLL2_OUT_MAIN, 14, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_mclk0_clk_src = {
	.cmd_rcgr = 0x15000,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_1,
	.freq_tbl = ftbl_cam_cc_mclk0_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_mclk0_clk_src",
		.parent_data = cam_cc_parent_data_1,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_1),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxa,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 68571429},
	},
};

static struct clk_rcg2 cam_cc_mclk1_clk_src = {
	.cmd_rcgr = 0x1501c,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_1,
	.freq_tbl = ftbl_cam_cc_mclk0_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_mclk1_clk_src",
		.parent_data = cam_cc_parent_data_1,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_1),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxa,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 68571429},
	},
};

static struct clk_rcg2 cam_cc_mclk2_clk_src = {
	.cmd_rcgr = 0x15038,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_1,
	.freq_tbl = ftbl_cam_cc_mclk0_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_mclk2_clk_src",
		.parent_data = cam_cc_parent_data_1,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_1),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxa,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 68571429},
	},
};

static struct clk_rcg2 cam_cc_mclk3_clk_src = {
	.cmd_rcgr = 0x15054,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_1,
	.freq_tbl = ftbl_cam_cc_mclk0_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_mclk3_clk_src",
		.parent_data = cam_cc_parent_data_1,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_1),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxa,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 68571429},
	},
};

static struct clk_rcg2 cam_cc_mclk4_clk_src = {
	.cmd_rcgr = 0x15070,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_1,
	.freq_tbl = ftbl_cam_cc_mclk0_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_mclk4_clk_src",
		.parent_data = cam_cc_parent_data_1,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_1),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxa,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 68571429},
	},
};

static struct clk_rcg2 cam_cc_mclk5_clk_src = {
	.cmd_rcgr = 0x1508c,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_1,
	.freq_tbl = ftbl_cam_cc_mclk0_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_mclk5_clk_src",
		.parent_data = cam_cc_parent_data_1,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_1),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxa,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 68571429},
	},
};

static struct clk_rcg2 cam_cc_mclk6_clk_src = {
	.cmd_rcgr = 0x150a8,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_1,
	.freq_tbl = ftbl_cam_cc_mclk0_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_mclk6_clk_src",
		.parent_data = cam_cc_parent_data_1,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_1),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxa,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 68571429},
	},
};

static struct clk_rcg2 cam_cc_mclk7_clk_src = {
	.cmd_rcgr = 0x150c4,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_1,
	.freq_tbl = ftbl_cam_cc_mclk0_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_mclk7_clk_src",
		.parent_data = cam_cc_parent_data_1,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_1),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mxa,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 68571429},
	},
};

static const struct freq_tbl ftbl_cam_cc_qdss_debug_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	F(75000000, P_CAM_CC_PLL0_OUT_EVEN, 8, 0, 0),
	F(150000000, P_CAM_CC_PLL0_OUT_EVEN, 4, 0, 0),
	F(300000000, P_CAM_CC_PLL0_OUT_MAIN, 4, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_qdss_debug_clk_src = {
	.cmd_rcgr = 0x1329c,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_qdss_debug_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_qdss_debug_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mm,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 75000000,
			[VDD_LOW] = 150000000,
			[VDD_LOW_L1] = 300000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_sfe_0_clk_src[] = {
	F(466000000, P_CAM_CC_PLL6_OUT_EVEN, 1, 0, 0),
	F(594000000, P_CAM_CC_PLL6_OUT_EVEN, 1, 0, 0),
	F(675000000, P_CAM_CC_PLL6_OUT_EVEN, 1, 0, 0),
	F(785000000, P_CAM_CC_PLL6_OUT_EVEN, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_sfe_0_clk_src = {
	.cmd_rcgr = 0x1306c,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_7,
	.freq_tbl = ftbl_cam_cc_sfe_0_clk_src,
	.enable_safe_config = true,
	.clkr = {
		.crm = &cam_crm,
		.crm_vcd = 3,
	},
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_sfe_0_clk_src",
		.parent_data = cam_cc_parent_data_7,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_7),
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
		.ops = &clk_rcg2_crmc_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 466000000,
			[VDD_LOW] = 594000000,
			[VDD_LOW_L1] = 675000000,
			[VDD_NOMINAL] = 785000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_sfe_1_clk_src[] = {
	F(466000000, P_CAM_CC_PLL7_OUT_EVEN, 1, 0, 0),
	F(594000000, P_CAM_CC_PLL7_OUT_EVEN, 1, 0, 0),
	F(675000000, P_CAM_CC_PLL7_OUT_EVEN, 1, 0, 0),
	F(785000000, P_CAM_CC_PLL7_OUT_EVEN, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_sfe_1_clk_src = {
	.cmd_rcgr = 0x130bc,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_8,
	.freq_tbl = ftbl_cam_cc_sfe_1_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr = {
		.crm = &cam_crm,
		.crm_vcd = 4,
	},
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_sfe_1_clk_src",
		.parent_data = cam_cc_parent_data_8,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_8),
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
		.ops = &clk_rcg2_crmc_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 466000000,
			[VDD_LOW] = 594000000,
			[VDD_LOW_L1] = 675000000,
			[VDD_NOMINAL] = 785000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_sfe_2_clk_src[] = {
	F(466000000, P_CAM_CC_PLL10_OUT_EVEN, 1, 0, 0),
	F(594000000, P_CAM_CC_PLL10_OUT_EVEN, 1, 0, 0),
	F(675000000, P_CAM_CC_PLL10_OUT_EVEN, 1, 0, 0),
	F(785000000, P_CAM_CC_PLL10_OUT_EVEN, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_sfe_2_clk_src = {
	.cmd_rcgr = 0x1310c,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_9,
	.freq_tbl = ftbl_cam_cc_sfe_2_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr = {
		.crm = &cam_crm,
		.crm_vcd = 5,
	},
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_sfe_2_clk_src",
		.parent_data = cam_cc_parent_data_9,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_9),
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
		.ops = &clk_rcg2_crmc_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 466000000,
			[VDD_LOW] = 594000000,
			[VDD_LOW_L1] = 675000000,
			[VDD_NOMINAL] = 785000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_sleep_clk_src[] = {
	F(32000, P_SLEEP_CLK, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_sleep_clk_src = {
	.cmd_rcgr = 0x132f0,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_10,
	.freq_tbl = ftbl_cam_cc_sleep_clk_src,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_sleep_clk_src",
		.parent_data = cam_cc_parent_data_10,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_10),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_class = &vdd_mm,
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 32000},
	},
};

static const struct freq_tbl ftbl_cam_cc_slow_ahb_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	F(80000000, P_CAM_CC_PLL0_OUT_EVEN, 7.5, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_slow_ahb_clk_src = {
	.cmd_rcgr = 0x10034,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_0,
	.freq_tbl = ftbl_cam_cc_slow_ahb_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_slow_ahb_clk_src",
		.parent_data = cam_cc_parent_data_0,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_0),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
	.clkr.vdd_data = {
		.vdd_classes = cam_cc_pineapple_regulators_1,
		.num_vdd_classes = ARRAY_SIZE(cam_cc_pineapple_regulators_1),
		.num_rate_max = VDD_NUM,
		.rate_max = (unsigned long[VDD_NUM]) {
			[VDD_LOWER] = 80000000},
	},
};

static const struct freq_tbl ftbl_cam_cc_xo_clk_src[] = {
	F(19200000, P_BI_TCXO, 1, 0, 0),
	{ }
};

static struct clk_rcg2 cam_cc_xo_clk_src = {
	.cmd_rcgr = 0x132d4,
	.mnd_width = 0,
	.hid_width = 5,
	.parent_map = cam_cc_parent_map_11,
	.freq_tbl = ftbl_cam_cc_xo_clk_src,
	.enable_safe_config = true,
	.flags = HW_CLK_CTRL_MODE,
	.clkr.hw.init = &(const struct clk_init_data){
		.name = "cam_cc_xo_clk_src",
		.parent_data = cam_cc_parent_data_11_ao,
		.num_parents = ARRAY_SIZE(cam_cc_parent_data_11_ao),
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch cam_cc_bps_ahb_clk = {
	.halt_reg = 0x1004c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1004c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_bps_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_slow_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_bps_clk = {
	.halt_reg = 0x10068,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x10068,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_bps_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_bps_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_bps_fast_ahb_clk = {
	.halt_reg = 0x10030,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x10030,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_bps_fast_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_fast_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_bps_shift_clk = {
	.halt_reg = 0x10078,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x10078,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_bps_shift_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_camnoc_axi_nrt_clk = {
	.halt_reg = 0x13284,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13284,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_camnoc_axi_nrt_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_camnoc_axi_rt_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_camnoc_axi_rt_clk = {
	.halt_reg = 0x13274,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13274,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_camnoc_axi_rt_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_camnoc_axi_rt_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_camnoc_dcd_xo_clk = {
	.halt_reg = 0x13290,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13290,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_camnoc_dcd_xo_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_camnoc_xo_clk = {
	.halt_reg = 0x13294,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13294,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_camnoc_xo_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_cci_0_clk = {
	.halt_reg = 0x131e4,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x131e4,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cci_0_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cci_0_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_cci_1_clk = {
	.halt_reg = 0x13200,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13200,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cci_1_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cci_1_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_cci_2_clk = {
	.halt_reg = 0x1321c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1321c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cci_2_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cci_2_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_core_ahb_clk = {
	.halt_reg = 0x132d0,
	.halt_check = BRANCH_HALT_DELAY,
	.clkr = {
		.enable_reg = 0x132d0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_core_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_slow_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_ahb_clk = {
	.halt_reg = 0x13220,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13220,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_slow_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_bps_clk = {
	.halt_reg = 0x10074,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x10074,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_bps_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_bps_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_cre_clk = {
	.halt_reg = 0x13160,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13160,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_cre_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cre_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_fast_ahb_clk = {
	.halt_reg = 0x1322c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1322c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_fast_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_fast_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_ife_0_clk = {
	.halt_reg = 0x1103c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1103c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_ife_0_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ife_0_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_ife_1_clk = {
	.halt_reg = 0x1203c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1203c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_ife_1_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ife_1_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_ife_2_clk = {
	.halt_reg = 0x1208c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1208c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_ife_2_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ife_2_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_ife_lite_clk = {
	.halt_reg = 0x13024,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13024,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_ife_lite_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ife_lite_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_ipe_nps_clk = {
	.halt_reg = 0x100b8,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x100b8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_ipe_nps_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ipe_nps_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_sbi_clk = {
	.halt_reg = 0x10104,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x10104,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_sbi_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ife_0_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_sfe_0_clk = {
	.halt_reg = 0x13090,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13090,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_sfe_0_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_sfe_0_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_sfe_1_clk = {
	.halt_reg = 0x130e0,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x130e0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_sfe_1_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_sfe_1_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_cpas_sfe_2_clk = {
	.halt_reg = 0x13130,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13130,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cpas_sfe_2_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_sfe_2_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_cre_ahb_clk = {
	.halt_reg = 0x13164,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13164,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cre_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_slow_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_cre_clk = {
	.halt_reg = 0x1315c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1315c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_cre_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cre_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_csi0phytimer_clk = {
	.halt_reg = 0x150f8,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x150f8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csi0phytimer_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_csi0phytimer_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_csi1phytimer_clk = {
	.halt_reg = 0x1511c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1511c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csi1phytimer_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_csi1phytimer_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_csi2phytimer_clk = {
	.halt_reg = 0x1513c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1513c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csi2phytimer_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_csi2phytimer_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_csi3phytimer_clk = {
	.halt_reg = 0x1515c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1515c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csi3phytimer_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_csi3phytimer_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_csi4phytimer_clk = {
	.halt_reg = 0x1517c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1517c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csi4phytimer_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_csi4phytimer_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_csi5phytimer_clk = {
	.halt_reg = 0x1519c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1519c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csi5phytimer_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_csi5phytimer_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_csi6phytimer_clk = {
	.halt_reg = 0x151bc,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x151bc,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csi6phytimer_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_csi6phytimer_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_csi7phytimer_clk = {
	.halt_reg = 0x151dc,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x151dc,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csi7phytimer_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_csi7phytimer_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_csid_clk = {
	.halt_reg = 0x13250,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13250,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csid_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_csid_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_csid_csiphy_rx_clk = {
	.halt_reg = 0x15100,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x15100,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csid_csiphy_rx_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cphy_rx_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_csiphy0_clk = {
	.halt_reg = 0x150fc,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x150fc,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csiphy0_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cphy_rx_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_csiphy1_clk = {
	.halt_reg = 0x15120,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x15120,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csiphy1_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cphy_rx_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_csiphy2_clk = {
	.halt_reg = 0x15140,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x15140,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csiphy2_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cphy_rx_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_csiphy3_clk = {
	.halt_reg = 0x15160,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x15160,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csiphy3_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cphy_rx_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_csiphy4_clk = {
	.halt_reg = 0x15180,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x15180,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csiphy4_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cphy_rx_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_csiphy5_clk = {
	.halt_reg = 0x151a0,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x151a0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csiphy5_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cphy_rx_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_csiphy6_clk = {
	.halt_reg = 0x151c0,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x151c0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csiphy6_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cphy_rx_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_csiphy7_clk = {
	.halt_reg = 0x151e0,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x151e0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_csiphy7_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cphy_rx_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_drv_ahb_clk = {
	.halt_reg = 0x13318,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13318,
		.enable_mask = BIT(0),
		.flags = QCOM_CLK_BOOT_CRITICAL | QCOM_CLK_IS_CRITICAL,
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_drv_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_slow_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_drv_xo_clk = {
	.halt_reg = 0x13314,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13314,
		.enable_mask = BIT(0),
		.flags = QCOM_CLK_BOOT_CRITICAL | QCOM_CLK_IS_CRITICAL,
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_drv_xo_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_icp_ahb_clk = {
	.halt_reg = 0x131c8,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x131c8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_icp_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_slow_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_icp_clk = {
	.halt_reg = 0x131bc,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x131bc,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_icp_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_icp_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_0_clk = {
	.halt_reg = 0x11030,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x11030,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_0_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ife_0_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_0_fast_ahb_clk = {
	.halt_reg = 0x11048,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x11048,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_0_fast_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_fast_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_0_shift_clk = {
	.halt_reg = 0x11064,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x11064,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_0_shift_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_1_clk = {
	.halt_reg = 0x12030,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x12030,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_1_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ife_1_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_1_fast_ahb_clk = {
	.halt_reg = 0x12048,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x12048,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_1_fast_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_fast_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_1_shift_clk = {
	.halt_reg = 0x1204c,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x1204c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_1_shift_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_2_clk = {
	.halt_reg = 0x12080,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x12080,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_2_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ife_2_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_2_fast_ahb_clk = {
	.halt_reg = 0x12098,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x12098,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_2_fast_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_fast_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_2_shift_clk = {
	.halt_reg = 0x1209c,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x1209c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_2_shift_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_lite_ahb_clk = {
	.halt_reg = 0x13050,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13050,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_lite_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_slow_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_lite_clk = {
	.halt_reg = 0x13018,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13018,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_lite_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ife_lite_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_lite_cphy_rx_clk = {
	.halt_reg = 0x1304c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1304c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_lite_cphy_rx_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_cphy_rx_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_ife_lite_csid_clk = {
	.halt_reg = 0x13040,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13040,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ife_lite_csid_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ife_lite_csid_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ipe_nps_ahb_clk = {
	.halt_reg = 0x100d0,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x100d0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ipe_nps_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_slow_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ipe_nps_clk = {
	.halt_reg = 0x100ac,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x100ac,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ipe_nps_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ipe_nps_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ipe_nps_fast_ahb_clk = {
	.halt_reg = 0x100d4,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x100d4,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ipe_nps_fast_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_fast_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ipe_pps_clk = {
	.halt_reg = 0x100bc,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x100bc,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ipe_pps_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ipe_nps_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ipe_pps_fast_ahb_clk = {
	.halt_reg = 0x100d8,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x100d8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ipe_pps_fast_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_fast_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_ipe_shift_clk = {
	.halt_reg = 0x100dc,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x100dc,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_ipe_shift_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_jpeg_1_clk = {
	.halt_reg = 0x1318c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1318c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_jpeg_1_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_jpeg_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_jpeg_clk = {
	.halt_reg = 0x13180,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13180,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_jpeg_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_jpeg_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_mclk0_clk = {
	.halt_reg = 0x15018,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x15018,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_mclk0_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_mclk0_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_mclk1_clk = {
	.halt_reg = 0x15034,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x15034,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_mclk1_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_mclk1_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_mclk2_clk = {
	.halt_reg = 0x15050,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x15050,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_mclk2_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_mclk2_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_mclk3_clk = {
	.halt_reg = 0x1506c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1506c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_mclk3_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_mclk3_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_mclk4_clk = {
	.halt_reg = 0x15088,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x15088,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_mclk4_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_mclk4_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_mclk5_clk = {
	.halt_reg = 0x150a4,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x150a4,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_mclk5_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_mclk5_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_mclk6_clk = {
	.halt_reg = 0x150c0,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x150c0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_mclk6_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_mclk6_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_mclk7_clk = {
	.halt_reg = 0x150dc,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x150dc,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_mclk7_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_mclk7_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_qdss_debug_clk = {
	.halt_reg = 0x132b4,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x132b4,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_qdss_debug_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_qdss_debug_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_qdss_debug_xo_clk = {
	.halt_reg = 0x132b8,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x132b8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_qdss_debug_xo_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_sbi_clk = {
	.halt_reg = 0x100f8,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x100f8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sbi_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_ife_0_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_sbi_fast_ahb_clk = {
	.halt_reg = 0x10108,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x10108,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sbi_fast_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_fast_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_sbi_shift_clk = {
	.halt_reg = 0x1010c,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x1010c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sbi_shift_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_sfe_0_clk = {
	.halt_reg = 0x13084,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13084,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sfe_0_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_sfe_0_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_sfe_0_fast_ahb_clk = {
	.halt_reg = 0x1309c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1309c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sfe_0_fast_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_fast_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_sfe_0_shift_clk = {
	.halt_reg = 0x130a0,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x130a0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sfe_0_shift_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_sfe_1_clk = {
	.halt_reg = 0x130d4,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x130d4,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sfe_1_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_sfe_1_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_sfe_1_fast_ahb_clk = {
	.halt_reg = 0x130ec,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x130ec,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sfe_1_fast_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_fast_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_sfe_1_shift_clk = {
	.halt_reg = 0x130f0,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x130f0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sfe_1_shift_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_sfe_2_clk = {
	.halt_reg = 0x13124,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13124,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sfe_2_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_sfe_2_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_crm_ops,
		},
	},
};

static struct clk_branch cam_cc_sfe_2_fast_ahb_clk = {
	.halt_reg = 0x1313c,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x1313c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sfe_2_fast_ahb_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_fast_ahb_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_sfe_2_shift_clk = {
	.halt_reg = 0x13140,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x13140,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sfe_2_shift_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_sleep_clk = {
	.halt_reg = 0x13308,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x13308,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_sleep_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_sleep_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch cam_cc_titan_top_shift_clk = {
	.halt_reg = 0x1330c,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x1330c,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data){
			.name = "cam_cc_titan_top_shift_clk",
			.parent_hws = (const struct clk_hw*[]){
				&cam_cc_xo_clk_src.clkr.hw,
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_regmap *cam_cc_pineapple_clocks[] = {
	[CAM_CC_BPS_AHB_CLK] = &cam_cc_bps_ahb_clk.clkr,
	[CAM_CC_BPS_CLK] = &cam_cc_bps_clk.clkr,
	[CAM_CC_BPS_CLK_SRC] = &cam_cc_bps_clk_src.clkr,
	[CAM_CC_BPS_FAST_AHB_CLK] = &cam_cc_bps_fast_ahb_clk.clkr,
	[CAM_CC_BPS_SHIFT_CLK] = &cam_cc_bps_shift_clk.clkr,
	[CAM_CC_CAMNOC_AXI_NRT_CLK] = &cam_cc_camnoc_axi_nrt_clk.clkr,
	[CAM_CC_CAMNOC_AXI_RT_CLK] = &cam_cc_camnoc_axi_rt_clk.clkr,
	[CAM_CC_CAMNOC_AXI_RT_CLK_SRC] = &cam_cc_camnoc_axi_rt_clk_src.clkr,
	[CAM_CC_CAMNOC_DCD_XO_CLK] = &cam_cc_camnoc_dcd_xo_clk.clkr,
	[CAM_CC_CAMNOC_XO_CLK] = &cam_cc_camnoc_xo_clk.clkr,
	[CAM_CC_CCI_0_CLK] = &cam_cc_cci_0_clk.clkr,
	[CAM_CC_CCI_0_CLK_SRC] = &cam_cc_cci_0_clk_src.clkr,
	[CAM_CC_CCI_1_CLK] = &cam_cc_cci_1_clk.clkr,
	[CAM_CC_CCI_1_CLK_SRC] = &cam_cc_cci_1_clk_src.clkr,
	[CAM_CC_CCI_2_CLK] = &cam_cc_cci_2_clk.clkr,
	[CAM_CC_CCI_2_CLK_SRC] = &cam_cc_cci_2_clk_src.clkr,
	[CAM_CC_CORE_AHB_CLK] = &cam_cc_core_ahb_clk.clkr,
	[CAM_CC_CPAS_AHB_CLK] = &cam_cc_cpas_ahb_clk.clkr,
	[CAM_CC_CPAS_BPS_CLK] = &cam_cc_cpas_bps_clk.clkr,
	[CAM_CC_CPAS_CRE_CLK] = &cam_cc_cpas_cre_clk.clkr,
	[CAM_CC_CPAS_FAST_AHB_CLK] = &cam_cc_cpas_fast_ahb_clk.clkr,
	[CAM_CC_CPAS_IFE_0_CLK] = &cam_cc_cpas_ife_0_clk.clkr,
	[CAM_CC_CPAS_IFE_1_CLK] = &cam_cc_cpas_ife_1_clk.clkr,
	[CAM_CC_CPAS_IFE_2_CLK] = &cam_cc_cpas_ife_2_clk.clkr,
	[CAM_CC_CPAS_IFE_LITE_CLK] = &cam_cc_cpas_ife_lite_clk.clkr,
	[CAM_CC_CPAS_IPE_NPS_CLK] = &cam_cc_cpas_ipe_nps_clk.clkr,
	[CAM_CC_CPAS_SBI_CLK] = &cam_cc_cpas_sbi_clk.clkr,
	[CAM_CC_CPAS_SFE_0_CLK] = &cam_cc_cpas_sfe_0_clk.clkr,
	[CAM_CC_CPAS_SFE_1_CLK] = &cam_cc_cpas_sfe_1_clk.clkr,
	[CAM_CC_CPAS_SFE_2_CLK] = &cam_cc_cpas_sfe_2_clk.clkr,
	[CAM_CC_CPHY_RX_CLK_SRC] = &cam_cc_cphy_rx_clk_src.clkr,
	[CAM_CC_CRE_AHB_CLK] = &cam_cc_cre_ahb_clk.clkr,
	[CAM_CC_CRE_CLK] = &cam_cc_cre_clk.clkr,
	[CAM_CC_CRE_CLK_SRC] = &cam_cc_cre_clk_src.clkr,
	[CAM_CC_CSI0PHYTIMER_CLK] = &cam_cc_csi0phytimer_clk.clkr,
	[CAM_CC_CSI0PHYTIMER_CLK_SRC] = &cam_cc_csi0phytimer_clk_src.clkr,
	[CAM_CC_CSI1PHYTIMER_CLK] = &cam_cc_csi1phytimer_clk.clkr,
	[CAM_CC_CSI1PHYTIMER_CLK_SRC] = &cam_cc_csi1phytimer_clk_src.clkr,
	[CAM_CC_CSI2PHYTIMER_CLK] = &cam_cc_csi2phytimer_clk.clkr,
	[CAM_CC_CSI2PHYTIMER_CLK_SRC] = &cam_cc_csi2phytimer_clk_src.clkr,
	[CAM_CC_CSI3PHYTIMER_CLK] = &cam_cc_csi3phytimer_clk.clkr,
	[CAM_CC_CSI3PHYTIMER_CLK_SRC] = &cam_cc_csi3phytimer_clk_src.clkr,
	[CAM_CC_CSI4PHYTIMER_CLK] = &cam_cc_csi4phytimer_clk.clkr,
	[CAM_CC_CSI4PHYTIMER_CLK_SRC] = &cam_cc_csi4phytimer_clk_src.clkr,
	[CAM_CC_CSI5PHYTIMER_CLK] = &cam_cc_csi5phytimer_clk.clkr,
	[CAM_CC_CSI5PHYTIMER_CLK_SRC] = &cam_cc_csi5phytimer_clk_src.clkr,
	[CAM_CC_CSI6PHYTIMER_CLK] = &cam_cc_csi6phytimer_clk.clkr,
	[CAM_CC_CSI6PHYTIMER_CLK_SRC] = &cam_cc_csi6phytimer_clk_src.clkr,
	[CAM_CC_CSI7PHYTIMER_CLK] = &cam_cc_csi7phytimer_clk.clkr,
	[CAM_CC_CSI7PHYTIMER_CLK_SRC] = &cam_cc_csi7phytimer_clk_src.clkr,
	[CAM_CC_CSID_CLK] = &cam_cc_csid_clk.clkr,
	[CAM_CC_CSID_CLK_SRC] = &cam_cc_csid_clk_src.clkr,
	[CAM_CC_CSID_CSIPHY_RX_CLK] = &cam_cc_csid_csiphy_rx_clk.clkr,
	[CAM_CC_CSIPHY0_CLK] = &cam_cc_csiphy0_clk.clkr,
	[CAM_CC_CSIPHY1_CLK] = &cam_cc_csiphy1_clk.clkr,
	[CAM_CC_CSIPHY2_CLK] = &cam_cc_csiphy2_clk.clkr,
	[CAM_CC_CSIPHY3_CLK] = &cam_cc_csiphy3_clk.clkr,
	[CAM_CC_CSIPHY4_CLK] = &cam_cc_csiphy4_clk.clkr,
	[CAM_CC_CSIPHY5_CLK] = &cam_cc_csiphy5_clk.clkr,
	[CAM_CC_CSIPHY6_CLK] = &cam_cc_csiphy6_clk.clkr,
	[CAM_CC_CSIPHY7_CLK] = &cam_cc_csiphy7_clk.clkr,
	[CAM_CC_DRV_AHB_CLK] = &cam_cc_drv_ahb_clk.clkr,
	[CAM_CC_DRV_XO_CLK] = &cam_cc_drv_xo_clk.clkr,
	[CAM_CC_FAST_AHB_CLK_SRC] = &cam_cc_fast_ahb_clk_src.clkr,
	[CAM_CC_ICP_AHB_CLK] = &cam_cc_icp_ahb_clk.clkr,
	[CAM_CC_ICP_CLK] = &cam_cc_icp_clk.clkr,
	[CAM_CC_ICP_CLK_SRC] = &cam_cc_icp_clk_src.clkr,
	[CAM_CC_IFE_0_CLK] = &cam_cc_ife_0_clk.clkr,
	[CAM_CC_IFE_0_CLK_SRC] = &cam_cc_ife_0_clk_src.clkr,
	[CAM_CC_IFE_0_FAST_AHB_CLK] = &cam_cc_ife_0_fast_ahb_clk.clkr,
	[CAM_CC_IFE_0_SHIFT_CLK] = &cam_cc_ife_0_shift_clk.clkr,
	[CAM_CC_IFE_1_CLK] = &cam_cc_ife_1_clk.clkr,
	[CAM_CC_IFE_1_CLK_SRC] = &cam_cc_ife_1_clk_src.clkr,
	[CAM_CC_IFE_1_FAST_AHB_CLK] = &cam_cc_ife_1_fast_ahb_clk.clkr,
	[CAM_CC_IFE_1_SHIFT_CLK] = &cam_cc_ife_1_shift_clk.clkr,
	[CAM_CC_IFE_2_CLK] = &cam_cc_ife_2_clk.clkr,
	[CAM_CC_IFE_2_CLK_SRC] = &cam_cc_ife_2_clk_src.clkr,
	[CAM_CC_IFE_2_FAST_AHB_CLK] = &cam_cc_ife_2_fast_ahb_clk.clkr,
	[CAM_CC_IFE_2_SHIFT_CLK] = &cam_cc_ife_2_shift_clk.clkr,
	[CAM_CC_IFE_LITE_AHB_CLK] = &cam_cc_ife_lite_ahb_clk.clkr,
	[CAM_CC_IFE_LITE_CLK] = &cam_cc_ife_lite_clk.clkr,
	[CAM_CC_IFE_LITE_CLK_SRC] = &cam_cc_ife_lite_clk_src.clkr,
	[CAM_CC_IFE_LITE_CPHY_RX_CLK] = &cam_cc_ife_lite_cphy_rx_clk.clkr,
	[CAM_CC_IFE_LITE_CSID_CLK] = &cam_cc_ife_lite_csid_clk.clkr,
	[CAM_CC_IFE_LITE_CSID_CLK_SRC] = &cam_cc_ife_lite_csid_clk_src.clkr,
	[CAM_CC_IPE_NPS_AHB_CLK] = &cam_cc_ipe_nps_ahb_clk.clkr,
	[CAM_CC_IPE_NPS_CLK] = &cam_cc_ipe_nps_clk.clkr,
	[CAM_CC_IPE_NPS_CLK_SRC] = &cam_cc_ipe_nps_clk_src.clkr,
	[CAM_CC_IPE_NPS_FAST_AHB_CLK] = &cam_cc_ipe_nps_fast_ahb_clk.clkr,
	[CAM_CC_IPE_PPS_CLK] = &cam_cc_ipe_pps_clk.clkr,
	[CAM_CC_IPE_PPS_FAST_AHB_CLK] = &cam_cc_ipe_pps_fast_ahb_clk.clkr,
	[CAM_CC_IPE_SHIFT_CLK] = &cam_cc_ipe_shift_clk.clkr,
	[CAM_CC_JPEG_1_CLK] = &cam_cc_jpeg_1_clk.clkr,
	[CAM_CC_JPEG_CLK] = &cam_cc_jpeg_clk.clkr,
	[CAM_CC_JPEG_CLK_SRC] = &cam_cc_jpeg_clk_src.clkr,
	[CAM_CC_MCLK0_CLK] = &cam_cc_mclk0_clk.clkr,
	[CAM_CC_MCLK0_CLK_SRC] = &cam_cc_mclk0_clk_src.clkr,
	[CAM_CC_MCLK1_CLK] = &cam_cc_mclk1_clk.clkr,
	[CAM_CC_MCLK1_CLK_SRC] = &cam_cc_mclk1_clk_src.clkr,
	[CAM_CC_MCLK2_CLK] = &cam_cc_mclk2_clk.clkr,
	[CAM_CC_MCLK2_CLK_SRC] = &cam_cc_mclk2_clk_src.clkr,
	[CAM_CC_MCLK3_CLK] = &cam_cc_mclk3_clk.clkr,
	[CAM_CC_MCLK3_CLK_SRC] = &cam_cc_mclk3_clk_src.clkr,
	[CAM_CC_MCLK4_CLK] = &cam_cc_mclk4_clk.clkr,
	[CAM_CC_MCLK4_CLK_SRC] = &cam_cc_mclk4_clk_src.clkr,
	[CAM_CC_MCLK5_CLK] = &cam_cc_mclk5_clk.clkr,
	[CAM_CC_MCLK5_CLK_SRC] = &cam_cc_mclk5_clk_src.clkr,
	[CAM_CC_MCLK6_CLK] = &cam_cc_mclk6_clk.clkr,
	[CAM_CC_MCLK6_CLK_SRC] = &cam_cc_mclk6_clk_src.clkr,
	[CAM_CC_MCLK7_CLK] = &cam_cc_mclk7_clk.clkr,
	[CAM_CC_MCLK7_CLK_SRC] = &cam_cc_mclk7_clk_src.clkr,
	[CAM_CC_PLL0] = &cam_cc_pll0.clkr,
	[CAM_CC_PLL0_OUT_EVEN] = &cam_cc_pll0_out_even.clkr,
	[CAM_CC_PLL0_OUT_ODD] = &cam_cc_pll0_out_odd.clkr,
	[CAM_CC_PLL1] = &cam_cc_pll1.clkr,
	[CAM_CC_PLL10] = &cam_cc_pll10.clkr,
	[CAM_CC_PLL10_OUT_EVEN] = &cam_cc_pll10_out_even.clkr,
	[CAM_CC_PLL1_OUT_EVEN] = &cam_cc_pll1_out_even.clkr,
	[CAM_CC_PLL2] = &cam_cc_pll2.clkr,
	[CAM_CC_PLL3] = &cam_cc_pll3.clkr,
	[CAM_CC_PLL3_OUT_EVEN] = &cam_cc_pll3_out_even.clkr,
	[CAM_CC_PLL4] = &cam_cc_pll4.clkr,
	[CAM_CC_PLL4_OUT_EVEN] = &cam_cc_pll4_out_even.clkr,
	[CAM_CC_PLL5] = &cam_cc_pll5.clkr,
	[CAM_CC_PLL5_OUT_EVEN] = &cam_cc_pll5_out_even.clkr,
	[CAM_CC_PLL6] = &cam_cc_pll6.clkr,
	[CAM_CC_PLL6_OUT_EVEN] = &cam_cc_pll6_out_even.clkr,
	[CAM_CC_PLL7] = &cam_cc_pll7.clkr,
	[CAM_CC_PLL7_OUT_EVEN] = &cam_cc_pll7_out_even.clkr,
	[CAM_CC_PLL8] = &cam_cc_pll8.clkr,
	[CAM_CC_PLL8_OUT_EVEN] = &cam_cc_pll8_out_even.clkr,
	[CAM_CC_PLL9] = &cam_cc_pll9.clkr,
	[CAM_CC_PLL9_OUT_EVEN] = &cam_cc_pll9_out_even.clkr,
	[CAM_CC_PLL9_OUT_ODD] = &cam_cc_pll9_out_odd.clkr,
	[CAM_CC_QDSS_DEBUG_CLK] = &cam_cc_qdss_debug_clk.clkr,
	[CAM_CC_QDSS_DEBUG_CLK_SRC] = &cam_cc_qdss_debug_clk_src.clkr,
	[CAM_CC_QDSS_DEBUG_XO_CLK] = &cam_cc_qdss_debug_xo_clk.clkr,
	[CAM_CC_SBI_CLK] = &cam_cc_sbi_clk.clkr,
	[CAM_CC_SBI_FAST_AHB_CLK] = &cam_cc_sbi_fast_ahb_clk.clkr,
	[CAM_CC_SBI_SHIFT_CLK] = &cam_cc_sbi_shift_clk.clkr,
	[CAM_CC_SFE_0_CLK] = &cam_cc_sfe_0_clk.clkr,
	[CAM_CC_SFE_0_CLK_SRC] = &cam_cc_sfe_0_clk_src.clkr,
	[CAM_CC_SFE_0_FAST_AHB_CLK] = &cam_cc_sfe_0_fast_ahb_clk.clkr,
	[CAM_CC_SFE_0_SHIFT_CLK] = &cam_cc_sfe_0_shift_clk.clkr,
	[CAM_CC_SFE_1_CLK] = &cam_cc_sfe_1_clk.clkr,
	[CAM_CC_SFE_1_CLK_SRC] = &cam_cc_sfe_1_clk_src.clkr,
	[CAM_CC_SFE_1_FAST_AHB_CLK] = &cam_cc_sfe_1_fast_ahb_clk.clkr,
	[CAM_CC_SFE_1_SHIFT_CLK] = &cam_cc_sfe_1_shift_clk.clkr,
	[CAM_CC_SFE_2_CLK] = &cam_cc_sfe_2_clk.clkr,
	[CAM_CC_SFE_2_CLK_SRC] = &cam_cc_sfe_2_clk_src.clkr,
	[CAM_CC_SFE_2_FAST_AHB_CLK] = &cam_cc_sfe_2_fast_ahb_clk.clkr,
	[CAM_CC_SFE_2_SHIFT_CLK] = &cam_cc_sfe_2_shift_clk.clkr,
	[CAM_CC_SLEEP_CLK] = &cam_cc_sleep_clk.clkr,
	[CAM_CC_SLEEP_CLK_SRC] = &cam_cc_sleep_clk_src.clkr,
	[CAM_CC_SLOW_AHB_CLK_SRC] = &cam_cc_slow_ahb_clk_src.clkr,
	[CAM_CC_TITAN_TOP_SHIFT_CLK] = &cam_cc_titan_top_shift_clk.clkr,
	[CAM_CC_XO_CLK_SRC] = &cam_cc_xo_clk_src.clkr,
};

static const struct qcom_reset_map cam_cc_pineapple_resets[] = {
	[CAM_CC_BPS_BCR] = { 0x10000 },
	[CAM_CC_DRV_BCR] = { 0x13310 },
	[CAM_CC_ICP_BCR] = { 0x131a0 },
	[CAM_CC_IFE_0_BCR] = { 0x11000 },
	[CAM_CC_IFE_1_BCR] = { 0x12000 },
	[CAM_CC_IFE_2_BCR] = { 0x12050 },
	[CAM_CC_IPE_0_BCR] = { 0x1007c },
	[CAM_CC_QDSS_DEBUG_BCR] = { 0x13298 },
	[CAM_CC_SBI_BCR] = { 0x100e0 },
	[CAM_CC_SFE_0_BCR] = { 0x13054 },
	[CAM_CC_SFE_1_BCR] = { 0x130a4 },
	[CAM_CC_SFE_2_BCR] = { 0x130f4 },
};

static const struct regmap_config cam_cc_pineapple_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = 0x1603c,
	.fast_io = true,
};

static struct qcom_cc_desc cam_cc_pineapple_desc = {
	.config = &cam_cc_pineapple_regmap_config,
	.clks = cam_cc_pineapple_clocks,
	.num_clks = ARRAY_SIZE(cam_cc_pineapple_clocks),
	.resets = cam_cc_pineapple_resets,
	.num_resets = ARRAY_SIZE(cam_cc_pineapple_resets),
	.clk_regulators = cam_cc_pineapple_regulators,
	.num_clk_regulators = ARRAY_SIZE(cam_cc_pineapple_regulators),
};

static const struct of_device_id cam_cc_pineapple_match_table[] = {
	{ .compatible = "qcom,pineapple-camcc" },
	{ .compatible = "qcom,pineapple-camcc-v2" },
	{ }
};
MODULE_DEVICE_TABLE(of, cam_cc_pineapple_match_table);

static void cam_cc_pineapple_fixup_pineapplev2(struct regmap *regmap)
{
	clk_lucid_ole_pll_configure(&cam_cc_pll1, regmap, &cam_cc_pll1_config_pineapple_v2);
	cam_cc_ipe_nps_clk_src.freq_tbl = ftbl_cam_cc_ipe_nps_clk_src_pineapple_v2;
	cam_cc_ipe_nps_clk_src.clkr.vdd_data.rate_max[VDD_LOWER] = 475000000;
}

static int cam_cc_pineapple_fixup(struct platform_device *pdev, struct regmap *regmap)
{
	const char *compat = NULL;
	int compatlen = 0;

	compat = of_get_property(pdev->dev.of_node, "compatible", &compatlen);
	if (!compat || compatlen <= 0)
		return -EINVAL;

	if (!strcmp(compat, "qcom,pineapple-camcc-v2"))
		cam_cc_pineapple_fixup_pineapplev2(regmap);

	return 0;
}

static int cam_cc_pineapple_probe(struct platform_device *pdev)
{
	struct regmap *regmap;
	int ret;

	regmap = qcom_cc_map(pdev, &cam_cc_pineapple_desc);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	ret = qcom_cc_runtime_init(pdev, &cam_cc_pineapple_desc);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(&pdev->dev);
	if (ret)
		return ret;

	clk_lucid_ole_pll_configure(&cam_cc_pll0, regmap, &cam_cc_pll0_config);
	clk_lucid_ole_pll_configure(&cam_cc_pll1, regmap, &cam_cc_pll1_config);
	clk_lucid_ole_pll_configure(&cam_cc_pll10, regmap, &cam_cc_pll10_config);
	clk_rivian_ole_pll_configure(&cam_cc_pll2, regmap, &cam_cc_pll2_config);
	clk_lucid_ole_pll_configure(&cam_cc_pll3, regmap, &cam_cc_pll3_config);
	clk_lucid_ole_pll_configure(&cam_cc_pll4, regmap, &cam_cc_pll4_config);
	clk_lucid_ole_pll_configure(&cam_cc_pll5, regmap, &cam_cc_pll5_config);
	clk_lucid_ole_pll_configure(&cam_cc_pll6, regmap, &cam_cc_pll6_config);
	clk_lucid_ole_pll_configure(&cam_cc_pll7, regmap, &cam_cc_pll7_config);
	clk_lucid_ole_pll_configure(&cam_cc_pll8, regmap, &cam_cc_pll8_config);
	clk_lucid_ole_pll_configure(&cam_cc_pll9, regmap, &cam_cc_pll9_config);

	ret = cam_cc_pineapple_fixup(pdev, regmap);
	if (ret)
		return ret;

	/*
	 * Keep clocks always enabled:
	 *	cam_cc_gdsc_clk
	 */
	regmap_update_bits(regmap, 0x132ec, BIT(0), BIT(0));

	ret = qcom_cc_really_probe(pdev, &cam_cc_pineapple_desc, regmap);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register CAM CC clocks\n");
		return ret;
	}

	pm_runtime_put_sync(&pdev->dev);
	dev_info(&pdev->dev, "Registered CAM CC clocks\n");

	return ret;
}

static void cam_cc_pineapple_sync_state(struct device *dev)
{
	qcom_cc_sync_state(dev, &cam_cc_pineapple_desc);
}

static const struct dev_pm_ops cam_cc_pineapple_pm_ops = {
	SET_RUNTIME_PM_OPS(qcom_cc_runtime_suspend, qcom_cc_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
};

static struct platform_driver cam_cc_pineapple_driver = {
	.probe = cam_cc_pineapple_probe,
	.driver = {
		.name = "cam_cc-pineapple",
		.of_match_table = cam_cc_pineapple_match_table,
		.sync_state = cam_cc_pineapple_sync_state,
		.pm = &cam_cc_pineapple_pm_ops,
	},
};

static int __init cam_cc_pineapple_init(void)
{
	return platform_driver_register(&cam_cc_pineapple_driver);
}
subsys_initcall(cam_cc_pineapple_init);

static void __exit cam_cc_pineapple_exit(void)
{
	platform_driver_unregister(&cam_cc_pineapple_driver);
}
module_exit(cam_cc_pineapple_exit);

MODULE_DESCRIPTION("QTI CAM_CC PINEAPPLE Driver");
MODULE_LICENSE("GPL");
