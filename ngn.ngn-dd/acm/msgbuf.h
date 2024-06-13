/* SPDX-License-Identifier: GPL-2.0
 *
 * TTTech ACM Linux driver
 * Copyright(c) 2019 TTTech Industrial Automation AG.
 *
 * Contact Information:
 * support@tttech-industrial.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */
/**
 * @file msgbuf.h
 * @brief ACM Driver Message Buffer Access
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_MSGBUF_H_
#define ACM_MSGBUF_H_

/**
 * @addtogroup hwaccmsgbuf
 * @{
 */

#include <linux/kernel.h>
#include <linux/uaccess.h>

/**
 * @brief message buffer descriptor type alias
 */
typedef u32 msgbuf_desc_t;

/**
 * @brief message buffer types
 */
enum acm_msgbuf_type {
	ACM_MSGBUF_TYPE_RX = 0,
	ACM_MSGBUF_TYPE_TX = 1
};

void msgbuf_read_lock_ctl(struct msgbuf *msgbuf,
	struct acmdrv_msgbuf_lock_ctrl *lock);
void msgbuf_write_lock_ctl(struct msgbuf *msgbuf,
	const struct acmdrv_msgbuf_lock_ctrl *lock);
void msgbuf_set_lock_ctl_mask(struct msgbuf *msgbuf,
	const struct acmdrv_msgbuf_lock_ctrl *mask);
void msgbuf_unset_lock_ctl_mask(struct msgbuf *msgbuf,
	const struct acmdrv_msgbuf_lock_ctrl *mask);
const struct acmdrv_msgbuf_lock_ctrl *
	msgbuf_get_lock_ctrl_mask(struct msgbuf *msgbuf);
u32 msgbuf_read_clear_overwritten(struct msgbuf *msgbuf, int i);
u32 msgbuf_read_status(struct msgbuf *msgbuf, int i);

size_t msgbuf_size(const struct msgbuf *msgbuf, int i);
enum acm_msgbuf_type msgbuf_type(const struct msgbuf *msgbuf, int i);
int __must_check msgbuf_desc_read(struct msgbuf *msgbuf,
				      unsigned int first, unsigned int last,
				      msgbuf_desc_t *buf);
int __must_check msgbuf_desc_write(struct msgbuf *msgbuf,
				       unsigned int first, unsigned int last,
				       msgbuf_desc_t *buf);
bool msgbuf_is_empty(struct msgbuf *msgbuf, int i);
bool msgbuf_is_valid(const struct msgbuf *msgbuf, int i);
int __must_check msgbuf_read_to_user(struct msgbuf *msgbuf, int i,
					 char __user *to, size_t size);
int __must_check msgbuf_write_from_user(struct msgbuf *msgbuf, int i,
					    const char __user *from,
					    size_t size);
void msgbuf_cleanup(struct msgbuf *msgbuf);

int __must_check msgbuf_init(struct acm *acm);
void msgbuf_exit(struct acm *acm);

/**@} hwaccmsgbuf */

#endif /* ACM_MSGBUF_H_ */
