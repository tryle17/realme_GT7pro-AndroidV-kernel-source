// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/bitops.h>
#include <linux/export.h>
#include <linux/regmap.h>
#include <linux/reset-controller.h>
#include <linux/delay.h>

#include "reset.h"

static void qcom_reset_delay(const struct qcom_reset_map *map)
{
	/*
	 * XO div-4 is commonly used for the reset demets, so by default allow
	 * enough time for 4 demet cycles at 1.2MHz.
	 */
	fsleep(map->udelay ?: 4);
}

static int qcom_reset(struct reset_controller_dev *rcdev, unsigned long id)
{
	struct qcom_reset_controller *rst = to_qcom_reset_controller(rcdev);

	rcdev->ops->assert(rcdev, id);
	qcom_reset_delay(&rst->reset_map[id]);
	rcdev->ops->deassert(rcdev, id);
	return 0;
}

static int
qcom_update_reset(struct qcom_reset_controller *rst, const struct qcom_reset_map *map,
		  u32 mask, u32 val)
{
	int ret;

	ret = regmap_update_bits(rst->regmap, map->reg, mask, val);
	if (ret)
		return ret;

	/* Ensure the write is fully propagated to the register. */
	ret = regmap_read(rst->regmap, map->reg, &val);
	if (ret)
		return ret;

	/* Give demets a chance to propagate the signal. */
	qcom_reset_delay(map);

	return 0;
}

static int
qcom_reset_assert(struct reset_controller_dev *rcdev, unsigned long id)
{
	struct qcom_reset_controller *rst;
	const struct qcom_reset_map *map;
	u32 mask;

	rst = to_qcom_reset_controller(rcdev);
	map = &rst->reset_map[id];
	mask = map->bitmask ? map->bitmask : BIT(map->bit);

	return qcom_update_reset(rst, map, mask, mask);
}

static int
qcom_reset_deassert(struct reset_controller_dev *rcdev, unsigned long id)
{
	struct qcom_reset_controller *rst;
	const struct qcom_reset_map *map;
	u32 mask;

	rst = to_qcom_reset_controller(rcdev);
	map = &rst->reset_map[id];
	mask = map->bitmask ? map->bitmask : BIT(map->bit);

	return qcom_update_reset(rst, map, mask, 0);
}

const struct reset_control_ops qcom_reset_ops = {
	.reset = qcom_reset,
	.assert = qcom_reset_assert,
	.deassert = qcom_reset_deassert,
};
EXPORT_SYMBOL_GPL(qcom_reset_ops);
