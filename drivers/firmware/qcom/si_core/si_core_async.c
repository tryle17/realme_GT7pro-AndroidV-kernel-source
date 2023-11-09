// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#include "si_core.h"

/* Processing of the async messges happens without any ordering. */

struct si_object *erase_si_object(u32 idx);

/* Async handlers and providers. */
struct async_msg {
	struct {
		u32 version;	/* Protocol version: top 16b major, lower 16b minor. */
		u32 op;			/* Async operation. */
	} header;

	/* Format of the Async data field is defined by the specified operation. */

	union {
		struct {
			u32 count;
			u32 obj[];
		} op_release;

		/* There are other structs but we do not care. */
		/* struct { XXX }; */
	};
};

/* Async Operations and header information. */

#define ASYNC_HEADER_SIZE sizeof(((struct async_msg *)(0))->header)

/* ASYNC_OP_x: operation.
 * ASYNC_OP_x_HDR_SIZE: header size for the operation.
 * ASYNC_OP_x_SIZE: size of each entry in a message for the operation.
 * ASYNC_OP_x_MSG_SIZE: size of a message with n entries.
 */

#define ASYNC_OP_RELEASE SI_OBJECT_OP_RELEASE	/* Added in minor version 0x0000. **/
#define ASYNC_OP_RELEASE_HDR_SIZE offsetof(struct async_msg, op_release.obj)
#define ASYNC_OP_RELEASE_SIZE sizeof(((struct async_msg *)(0))->op_release.obj[0])
#define ASYNC_OP_RELEASE_MSG_SIZE(n) \
	(ASYNC_OP_RELEASE_HDR_SIZE + ((n) * ASYNC_OP_RELEASE_SIZE))

/* 'async_si_buffer' return the available async buffer in the output buffer. */

static struct si_buffer async_si_buffer(struct si_object_invoke_ctx *oic)
{
	int i;
	size_t offset;

	struct qtee_callback *msg = (struct qtee_callback *)oic->out.msg.addr;

	if (!(oic->flags & OIC_FLAG_BUSY))
		return oic->out.msg;

	/* Async requests are appended to the output buffer after the CB message. */

	offset = OFFSET_TO_BUFFER_ARGS(msg, counts_total(msg->counts));

	for_each_input_buffer(i, msg->counts)
		offset += align_offset(msg->args[i].b.size);

	for_each_output_buffer(i, msg->counts)
		offset += align_offset(msg->args[i].b.size);

	if (oic->out.msg.size > offset) {
		return (struct si_buffer)
			{
				oic->out.msg.addr + offset,
				oic->out.msg.size - offset
			};
	}

	pr_err("no space left for async messages! or malformed message.\n");

	return (struct si_buffer) { 0, 0 };
}

#ifndef CONFIG_QCOM_SI_CORE_WQ

/* 'async_ops_mutex' serialize the construction of async message. */
static DEFINE_MUTEX(async_ops_mutex);

/* List of objects that should be released. */
/* All objects in 'release_ops_list' should have refcount set to ''zero''. */
static LIST_HEAD(release_ops_list);

/* 'release_user_object' put object in release pending list.
 * 'async_release_provider' remove objects from release pending list and construct
 *  the async message.
 * 'destroy_user_object' called to finish the job after QTEE acknowledged the release.
 */

void release_user_object(struct si_object *object)
{
	/* Use async message for RELEASE request.
	 * We free the object in '__release__async_queued_reqs' when appropriate.
	 */

	pr_debug("%s queued for async release.\n", si_object_name(object));

	mutex_lock(&async_ops_mutex);
	list_add_tail(&object->node, &release_ops_list);
	mutex_unlock(&async_ops_mutex);
}

static size_t async_release_provider(struct si_object_invoke_ctx *oic,
	struct async_msg *async_msg, size_t size)
{
	int i = 0;
	struct si_object *object, *t;

	/* We need space for at least a single entry. */
	if (size < ASYNC_OP_RELEASE_MSG_SIZE(1))
		return 0;

	mutex_lock(&async_ops_mutex);
	list_for_each_entry_safe(object, t, &release_ops_list, node) {
		async_msg->op_release.obj[i] = object->info.object_ptr;

		/* Move object to the oic's object_head.
		 * We only free objects in '__release__async_queued_reqs' if QTEE
		 * acknowledge the release; otherwise, move back objects to 'release_ops_list'
		 * in '__revive__async_queued_reqs'.
		 */

		list_move(&object->node, &oic->objects_head);

		if (size - ASYNC_OP_RELEASE_SIZE < ASYNC_OP_RELEASE_MSG_SIZE(++i))
			break;
	}

	mutex_unlock(&async_ops_mutex);

	/* INITIALIZE the header and message. */

	if (i) {
		async_msg->header.version = 0x00010002U;
		async_msg->header.op = ASYNC_OP_RELEASE;
		async_msg->op_release.count = i;
	}

	return (i) ? ASYNC_OP_RELEASE_MSG_SIZE(i) : 0;
}

static void destroy_user_object(struct si_object *object)
{
	kfree(object->name);

	/* QTEE release should be done! free the object. */
	free_si_object(object);
}

ssize_t release_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct si_object *object;
	size_t len = 0;

	mutex_lock(&async_ops_mutex);
	list_for_each_entry(object, &release_ops_list, node)
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s\n", si_object_name(object));
	mutex_unlock(&async_ops_mutex);

	return len;
}

/* '__append__async_reqs',
 * '__revive__async_queued_reqs', and
 * '__release__async_queued_reqs'.
 */

/* '__append__async_reqs' is a provider dispatcher (from si_core to QTEE). */

void __append__async_reqs(struct si_object_invoke_ctx *oic)
{
	struct si_buffer async_buffer = async_si_buffer(oic);

	pr_debug("%u.\n", oic->context_id);

	async_release_provider(oic, async_buffer.addr, async_buffer.size);

	/* TODO. call providers for any input objects in oic. */
}

void __revive__async_queued_reqs(struct si_object_invoke_ctx *oic)
{
	pr_debug("%u.\n", oic->context_id);

	mutex_lock(&async_ops_mutex);

	/* QTEE did not receive the object RELEASE requests.
	 * Put them back in 'release_ops_list' to retry again.
	 */

	/* TODO. Handle other requests. */

	list_splice(&oic->objects_head, &release_ops_list);

	INIT_LIST_HEAD(&oic->objects_head);
	mutex_unlock(&async_ops_mutex);
}

void __release__async_queued_reqs(struct si_object_invoke_ctx *oic)
{
	struct si_object *object, *t;

	pr_debug("%u.\n", oic->context_id);

	list_for_each_entry_safe(object, t, &oic->objects_head, node) {
		list_del(&object->node);

		destroy_user_object(object);

		/* TODO. Handle other requests. */
	}
}

#else
void __append__async_reqs(struct si_object_invoke_ctx *oic) { }
void __revive__async_queued_reqs(struct si_object_invoke_ctx *oic) { }
void __release__async_queued_reqs(struct si_object_invoke_ctx *oic) { }
#endif /* CONFIG_QCOM_SI_CORE_WQ */

static size_t async_release_handler(struct si_object_invoke_ctx *oic,
	struct async_msg *async_msg, size_t size)
{
	int i;

	/* We need space for at least a single entry. */
	if (size < ASYNC_OP_RELEASE_MSG_SIZE(1))
		return 0;

	for (i = 0; i < async_msg->op_release.count; i++) {
		struct si_object *object;

		/* Remove the 'object' from 'xa_si_objects' so that the 'object_id'
		 * becomes invalid for further use. However, call 'put_si_object'
		 * to schedule the actual release if there is no user.
		 */

		object = erase_si_object(async_msg->op_release.obj[i]);

		pr_debug("%s (refs: %u).\n", si_object_name(object), kref_read(&object->refcount));

		put_si_object(object);
	}

	return ASYNC_OP_RELEASE_MSG_SIZE(i);
}

/* '__fetch__async_reqs' is a handler dispatcher (from QTEE to si_core). */

void __fetch__async_reqs(struct si_object_invoke_ctx *oic)
{
	size_t consumed, used = 0;

	struct si_buffer async_buffer = async_si_buffer(oic);

	pr_debug("%u.\n", oic->context_id);

	while (async_buffer.size - used > ASYNC_HEADER_SIZE) {
		struct async_msg *async_msg = (struct async_msg *)(async_buffer.addr + used);

		/* QTEE assumes unused buffer is set to zero. */
		if (!async_msg->header.version)
			goto out;

		switch (async_msg->header.op) {
		case ASYNC_OP_RELEASE:
			consumed = async_release_handler(oic,
				async_msg, async_buffer.size - used);

			break;
		default: /* Unsupported operations. */
			consumed = 0;
		}

		used += align_offset(consumed);

		if (!consumed) {
			pr_err("Drop async buffer (context_id %d): buffer %p, (%p, %zx), processed %zx\n",
				oic->context_id,
				oic->out.msg.addr,	/* Address of Output buffer. */
				async_buffer.addr,	/* Address of beginning of async buffer. */
				async_buffer.size,	/* Available size of async buffer. */
				used);				/* Processed async buffer. */

			/* Dump a couple of lines from the buffer. */
			print_hex_dump(KERN_INFO, "si-core: ", DUMP_PREFIX_OFFSET, 16, 4,
				async_buffer.addr,
				min(ASYNC_HEADER_SIZE + 88, async_buffer.size),
				true);

			goto out;
		}
	}

 out:

	/* Reset the async buffer for next use in '__append__async_reqs'. */
	memset(async_buffer.addr, 0, async_buffer.size);
}
