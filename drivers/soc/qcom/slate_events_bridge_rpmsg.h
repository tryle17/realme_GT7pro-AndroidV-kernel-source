/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifndef SEB_RPMSG_H
#define SEB_RPMSG_H

#include <linux/rpmsg.h>
#include "slate_events_bridge.h"

struct seb_rpmsg_dev {
	struct rpmsg_endpoint *channel;
	struct device *dev;
	bool chnl_state;
	void *message;
	size_t message_length;
};

int seb_rpmsg_tx_msg(void  *msg, size_t len);

struct seb_channel_ops {
	void (*glink_channel_state)(bool state);
	void (*rx_msg)(void *data, int len);
};

void seb_channel_init(void (*fn1)(bool), void (*fn2)(void *, int));

#endif /* SEB_RPMSG_H */
