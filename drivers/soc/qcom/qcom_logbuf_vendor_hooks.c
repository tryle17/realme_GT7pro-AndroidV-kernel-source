// SPDX-License-Identifier: GPL-2.0-only

/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#define pr_fmt(fmt) "logbuf_vh: " fmt

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <soc/qcom/minidump.h>
#include <linux/kallsyms.h>

#include "../../../kernel/printk/printk_ringbuffer.h"
#include "debug_symbol.h"

void register_log_minidump(struct printk_ringbuffer *prb)
{
	struct prb_desc_ring descring;
	struct prb_data_ring textdata_ring;
	struct prb_desc *descaddr;
	struct printk_info *p_infos;
	struct md_region md_entry;
	int ret;
	static bool log_buf_registered;

	if (log_buf_registered || !prb)
		return;

	log_buf_registered = true;
	descring = prb->desc_ring;
	textdata_ring = prb->text_data_ring;
	descaddr = descring.descs;
	p_infos = descring.infos;

	strscpy(md_entry.name, "KLOGBUF", sizeof(md_entry.name));
	md_entry.virt_addr = (uintptr_t)textdata_ring.data;
	md_entry.phys_addr = virt_to_phys(textdata_ring.data);
	md_entry.size = _DATA_SIZE(textdata_ring.size_bits);
	ret = msm_minidump_add_region(&md_entry);
	if (ret < 0)
		pr_err("Failed to add log_text entry in minidump table\n");

	strscpy(md_entry.name, "LOG_DESC", sizeof(md_entry.name));
	md_entry.virt_addr = (uintptr_t)descaddr;
	md_entry.phys_addr = virt_to_phys(descaddr);
	md_entry.size = sizeof(struct prb_desc) * _DESCS_COUNT(descring.count_bits);
	ret = msm_minidump_add_region(&md_entry);
	if (ret < 0)
		pr_err("Failed to add log_desc entry in minidump table\n");

	strscpy(md_entry.name, "LOG_INFO", sizeof(md_entry.name));
	md_entry.virt_addr = (uintptr_t)p_infos;
	md_entry.phys_addr = virt_to_phys(p_infos);
	md_entry.size = sizeof(struct printk_info) * _DESCS_COUNT(descring.count_bits);
	ret = msm_minidump_add_region(&md_entry);
	if (ret < 0)
		pr_err("Failed to add log_info entry in minidump table\n");
}

static int __init logbuf_vh_driver_init(void)
{
	struct printk_ringbuffer *prb = NULL;

	if (!debug_symbol_available()) {
		pr_err("debug symbol driver is not available\n");
		return -ENODEV;
	}

	prb = *(struct printk_ringbuffer **)DEBUG_SYMBOL_LOOKUP(prb);
	register_log_minidump(prb);

	return 0;
}

module_init(logbuf_vh_driver_init);

MODULE_DESCRIPTION("QCOM Logbuf Vendor Hooks Driver");
MODULE_LICENSE("GPL");
