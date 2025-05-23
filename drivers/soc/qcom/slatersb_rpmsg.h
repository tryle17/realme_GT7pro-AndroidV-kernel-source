/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef SLATERSBRPMSG_H
#define SLATERSBRPMSG_H

#include <linux/rpmsg.h>
#include "slatersb.h"

struct slatersb_rpmsg_dev {
	struct rpmsg_endpoint *channel;
	struct device *dev;
	bool chnl_state;
	void *message;
	size_t message_length;
};

struct rsb_channel_ops {
	void (*glink_channel_state)(bool state);
	void (*rx_msg)(void *data, int len);
};

void slatersb_channel_init(void (*fn1)(bool), void (*fn2)(void *, int));


#if IS_ENABLED(CONFIG_MSM_SLATERSB_RPMSG)
int slatersb_rpmsg_tx_msg(void  *msg, size_t len);
#else
static inline int slatersb_rpmsg_tx_msg(void  *msg, size_t len)
{
	return -EIO;
}
#endif

#endif
