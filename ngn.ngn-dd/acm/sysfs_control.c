// SPDX-License-Identifier: GPL-2.0
/*
 * TTTech ACM Linux driver
 * Copyright(c) 2019 TTTech Industrial Automation AG.
 *
 * Contact Information:
 * support@tttech-industrial.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */
/**
 * @file sysfs_control.c
 * @brief ACM Driver SYSFS - Control Section
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup acmsysfscontrolimpl ACM SYSFS Control Implementation
 * @brief Implementation of SYSFS Control Section
 *
 * @{
 */

#include <linux/sysfs.h>
#include <asm/unaligned.h>

#include "acm-module.h"
#include "msgbuf.h"
#include "commreg.h"
#include "sysfs.h"
#include "sysfs_control.h"

/**
 * @brief Attribute read function for lock_msg_bufs
 */
static ssize_t lock_msg_bufs_read(struct file *filp, struct kobject *kobj,
				  struct bin_attribute *bin_attr, char *buf,
				  loff_t off, size_t size)
{
	struct acm *acm = kobj_to_acm(kobj);
	struct msgbuf *msgbuf = acm->msgbuf;
	struct acmdrv_msgbuf_lock_ctrl *lock =
		(struct acmdrv_msgbuf_lock_ctrl *)buf;

	/* access entire lock vector only */
	if ((off != 0) || size != sizeof(struct acmdrv_msgbuf_lock_ctrl))
		return -EINVAL;

	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(lock);
	msgbuf_read_lock_ctl(msgbuf, lock);
	ACMDRV_MSGBUF_LOCK_CTRL_AND(&lock, &lock,
		msgbuf_get_lock_ctrl_mask(msgbuf));

	return size;
}

/**
 * @brief Attribute write function for lock_msg_bufs
 */
static ssize_t lock_msg_bufs_write(struct file *filp, struct kobject *kobj,
				   struct bin_attribute *bin_attr, char *buf,
				   loff_t off, size_t size)
{
	struct acm *acm = kobj_to_acm(kobj);
	struct msgbuf *msgbuf = acm->msgbuf;
	struct acmdrv_msgbuf_lock_ctrl *lock =
		(struct acmdrv_msgbuf_lock_ctrl *)buf;

	/* access entire lock vector only */
	if ((off != 0) || size != sizeof(*lock))
		return -EINVAL;

	/* only access valid bits */
	msgbuf_set_lock_ctl_mask(msgbuf,
		ACMDRV_MSGBUF_LOCK_CTRL_AND(lock, lock,
			msgbuf_get_lock_ctrl_mask(msgbuf)));

	return size;
}

/**
 * @brief Attribute read function for unlock_msg_bufs
 */
static ssize_t unlock_msg_bufs_read(struct file *filp, struct kobject *kobj,
				    struct bin_attribute *bin_attr, char *buf,
				    loff_t off, size_t size)
{
	struct acm *acm = kobj_to_acm(kobj);
	struct msgbuf *msgbuf = acm->msgbuf;
	struct acmdrv_msgbuf_lock_ctrl *lock =
		(struct acmdrv_msgbuf_lock_ctrl *)buf;

	/* access entire lock vector only */
	if ((off != 0) || size != sizeof(*lock))
		return -EINVAL;

	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(lock);

	msgbuf_read_lock_ctl(msgbuf, lock);
	ACMDRV_MSGBUF_LOCK_CTRL_AND(lock,
		ACMDRV_MSGBUF_LOCK_CTRL_NOT(lock, lock),
		msgbuf_get_lock_ctrl_mask(msgbuf));

	return size;
}

/**
 * @brief Attribute write function for unlock_msg_bufs
 */
static ssize_t unlock_msg_bufs_write(struct file *filp, struct kobject *kobj,
				     struct bin_attribute *bin_attr, char *buf,
				     loff_t off, size_t size)
{
	struct acm *acm = kobj_to_acm(kobj);
	struct msgbuf *msgbuf = acm->msgbuf;
	struct acmdrv_msgbuf_lock_ctrl *lock =
		(struct acmdrv_msgbuf_lock_ctrl *)buf;

	/* access entire lock vector only */
	if ((off != 0) || size != sizeof(*lock))
		return -EINVAL;

	/* only access valid bits */
	msgbuf_unset_lock_ctl_mask(msgbuf,
		ACMDRV_MSGBUF_LOCK_CTRL_AND(lock, lock,
			msgbuf_get_lock_ctrl_mask(msgbuf)));
	msgbuf_unset_lock_ctl_mask(msgbuf, lock);

	return size;
}

/**
 * @brief Attribute read function for overwritten
 */
static ssize_t overwritten_read(struct file *filp, struct kobject *kobj,
				struct bin_attribute *bin_attr, char *buf,
				loff_t off, size_t size)
{
	int ret;
	int i;
	const off_t offs = off;
	struct acm *acm = kobj_to_acm(kobj);
	struct msgbuf *msgbuf = acm->msgbuf;

	ret = sysfs_bin_attr_check(bin_attr, off, size, sizeof(u32));
	if (ret)
		return ret;

	foreach_item(i, offs, size, sizeof(u32)) {
		u32 overwritten = msgbuf_read_clear_overwritten(msgbuf, i);

		put_unaligned(overwritten, (u32 *)buf);
		buf += sizeof(u32);
	}

	return size;
}

/**
 * @brief Control attribute lock_msg_bufs
 */
static BIN_ATTR_RW(lock_msg_bufs, sizeof(struct acmdrv_msgbuf_lock_ctrl));
/**
 * @brief Control attribute unlock_msg_bufs
 */
static BIN_ATTR_RW(unlock_msg_bufs, sizeof(struct acmdrv_msgbuf_lock_ctrl));
/**
 * @brief Control attribute overwritten
 */
static BIN_ATTR_RO(overwritten, 0 /* size set by init */);

/**
 * @brief sysfs attributes of control section
 */
static struct bin_attribute *control_attributes[] = {
	&bin_attr_lock_msg_bufs,
	&bin_attr_unlock_msg_bufs,
	&bin_attr_overwritten,
	NULL
};

/**
 * @brief Linux attribute group for control section
 */
static const struct attribute_group control_group = {
	.name = __stringify(ACMDRV_SYSFS_CONTROL_GROUP),
	.bin_attrs = control_attributes,
};

/**
 * @brief initialize sysfs control section
 *
 * @param acm ACM instance
 */
const struct attribute_group *sysfs_control_init(struct acm *acm)
{
	bin_attr_overwritten.size =
		commreg_read_msgbuf_count(acm->commreg) * sizeof(u32);

	return &control_group;
}

/**
 * @brief exit sysfs control section
 *
 * @param acm ACM instance
 */
void sysfs_control_exit(struct acm *acm)
{
	/* nothing to do */
}
/**@} acmsysfscontrolimpl */
