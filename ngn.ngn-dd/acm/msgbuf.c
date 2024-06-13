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
 * @file msgbuf.c
 * @brief ACM Driver Message Buffer Access
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup hwaccmsgbuf ACM IP Messagebuffers
 * @brief Provides access to the message buffer module IP
 *
 * @{
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of.h>

#include "acm-module.h"
#include "acmio.h"
#include "acmbitops.h"
#include "commreg.h"
#include "msgbuf.h"

/**
 * @name Message buffer Subsection Offsets
 * @brief Register offsets of the respective Message buffer Subsections
 */
/**@{*/
#define ACM_MSGBUF_DESC(i)	(0x00000000 + (i) * sizeof(msgbuf_desc_t))
#define ACM_MSGBUF_LOCK_CTL	0x00000200
#define ACM_MSGBUF_LOCK_CTL_LO	ACM_MSGBUF_LOCK_CTL
#define ACM_MSGBUF_LOCK_CTL_HI	(ACM_MSGBUF_LOCK_CTL + 4)
#define ACM_MSGBUF_RESET_CTL	0x00000300
#define ACM_MSGBUF_IRQ_CTL	0x00000400
#define ACM_MSGBUF_STATUS(i)	(0x00000600 + (i) * sizeof(u32))
#define ACM_MSGBUF_TIMESTAMP	0x00000800
#define ACM_MSGBUF_DATA_SIZE	0x800
#define ACM_MSGBUF_DATA(i)	(0x00010000 + (i) * ACM_MSGBUF_DATA_SIZE)
/**@}*/

/**
 * @name Message Buffer Descriptor
 * @brief Bit-Structure of Message Buffer Descriptor
 *
 * Use the API interface definitions since there is no deviation here.
 */
/**@{*/
#define ACM_MSGBUF_DESC_MSGBUF_OFFSET	ACM_BUFF_DESC_BUFF_OFFSET
#define ACM_MSGBUF_DESC_RESET		ACM_BUFF_DESC_BUFF_RST
#define ACM_MSGBUF_DESC_TYPE		ACMDRV_BUFF_DESC_BUFF_TYPE
#define ACM_MSGBUF_DESC_SUBBUFFER_SIZE	ACMDRV_BUFF_DESC_SUB_BUFF_SIZE
#define ACM_MSGBUF_DESC_RESERV2		BIT(30)
#define ACM_MSGBUF_DESC_VALID		ACMDRV_BUFF_DESC_VALID

#define ACM_MSGBUF_DESC_TYPE_RX		ACM_BUFF_DESC_BUFF_TYPE_RX
#define ACM_MSGBUF_DESC_TYPE_TX		ACM_BUFF_DESC_BUFF_TYPE_TX
/**@}*/

/**
 * @name Message Buffer Status
 * @brief Bit-Structure of Message Buffer Status
 */
/**@{*/
#define ACM_MSGBUF_STATUS_FCS		BIT(0)
#define ACM_MSGBUF_STATUS_DSCR_ERR	BIT(1)
#define ACM_MSGBUF_STATUS_OVERWRITTEN	BIT(2)
#define ACM_MSGBUF_STATUS_D_LOCKED	BIT(20)
#define ACM_MSGBUF_STATUS_FRESH		BIT(24)
#define ACM_MSGBUF_STATUS_EMPTY		BIT(28)
/**@}*/

/**
 * @brief message buffer handler instance
 */
struct msgbuf {
	void __iomem *base;		/**< base address */
	resource_size_t size;		/**< module size */
	struct acm *acm;		/**< associated ACM instance */

	atomic_t *overwritten;		/**< overwritten counter array */
	msgbuf_desc_t *desc_cache;	/**< cache for message buffer descs */

	struct mutex lock_ctl_lock;	/**< lock for lock_cnt access */
	struct mutex desc_lock;		/**< lock for descriptor access */

	struct mutex msgbuf_lock;	/**< lock for msgbuf access */
	struct acmdrv_msgbuf_lock_ctrl mask;	/**< mask for lock control */
};

/**
 * @brief read lock control bit field
 */
static void _msgbuf_read_lock_ctl(const struct msgbuf *msgbuf,
	struct acmdrv_msgbuf_lock_ctrl *lock)
{
	int i;
	struct acm *acm = msgbuf->acm;

	for (i = 0;
	     i < howmany(commreg_read_msgbuf_count(acm->commreg),
						   sizeof(u32) * NBBY);
	     ++i) {
		lock->bits[i] = readl(msgbuf->base + ACM_MSGBUF_LOCK_CTL
			+ i * sizeof(u32));
		rmb(); /* ensure read sequence on ACM IP */
	}
}

/**
 * @brief write lock control bit field
 */
static void _msgbuf_write_lock_ctl(struct msgbuf *msgbuf,
	const struct acmdrv_msgbuf_lock_ctrl *lock)
{
	int i;
	struct acm *acm = msgbuf->acm;

	for (i = 0;
	     i < howmany(commreg_read_msgbuf_count(acm->commreg),
						   sizeof(u32) * NBBY);
	     ++i) {
		writel(lock->bits[i], msgbuf->base + ACM_MSGBUF_LOCK_CTL
			+ i * sizeof(u32));
		wmb(); /* ensure write sequence on ACM IP */
	}

}

/**
 * @brief Update overwritten counter
 */
static void msgbuf_update_overwritten(struct msgbuf *msgbuf, int i,
				      u32 status)
{
	if (!(status & ACM_MSGBUF_STATUS_OVERWRITTEN))
		return;

	atomic_inc(&msgbuf->overwritten[i]);
}

/**
 * @brief read and clear overwritten counter of a messagebuffer
 */
u32 msgbuf_read_clear_overwritten(struct msgbuf *msgbuf, int i)
{
	return atomic_xchg(&msgbuf->overwritten[i], 0);
}
/**
 * read status of a message buffer
 */
u32 msgbuf_read_status(struct msgbuf *msgbuf, int i)
{
	u32 status;

	if (ACM_MSGBUF_STATUS(i) >= msgbuf->size) {
		dev_err(acm_dev(msgbuf->acm), "%s: access out of range: 0x%08zx\n",
			__func__, ACM_MSGBUF_STATUS(i));
		return 0;
	}

	status = readl(msgbuf->base + ACM_MSGBUF_STATUS(i));
	msgbuf_update_overwritten(msgbuf, i, status);

	return status;
}

/**
 * read message buffer descriptor
 */
static u32 _msgbuf_read_desc(struct msgbuf *msgbuf, int i)
{
	if (ACM_MSGBUF_DESC(i) >= msgbuf->size) {
		dev_err(acm_dev(msgbuf->acm), "%s: access out of range: 0x%08zx\n",
			__func__, ACM_MSGBUF_DESC(i));
		return 0;
	}

	mutex_lock(&msgbuf->desc_lock);
	msgbuf->desc_cache[i] = readl(msgbuf->base + ACM_MSGBUF_DESC(i));
	mutex_unlock(&msgbuf->desc_lock);

	return msgbuf->desc_cache[i];
}

/**
 * write message buffer descriptor
 */
static void _msgbuf_write_desc(struct msgbuf *msgbuf, int i, u32 value)
{
	if (ACM_MSGBUF_DESC(i) >= msgbuf->size) {
		dev_err(acm_dev(msgbuf->acm), "%s: access out of range: 0x%08zx\n",
			__func__, ACM_MSGBUF_DESC(i));
		return;
	}

	mutex_lock(&msgbuf->desc_lock);
	msgbuf->desc_cache[i] = value;
	writel(value, msgbuf->base + ACM_MSGBUF_DESC(i));
	mutex_unlock(&msgbuf->desc_lock);
}

/**
 * @brief read access to lock control
 */
void msgbuf_read_lock_ctl(struct msgbuf *msgbuf,
	struct acmdrv_msgbuf_lock_ctrl *lock)
{
	mutex_lock(&msgbuf->lock_ctl_lock);
	_msgbuf_read_lock_ctl(msgbuf, lock);
	mutex_unlock(&msgbuf->lock_ctl_lock);
}

/**
 * @brief write access to lock control
 */
void msgbuf_write_lock_ctl(struct msgbuf *msgbuf,
	const struct acmdrv_msgbuf_lock_ctrl *lock)
{
	mutex_lock(&msgbuf->lock_ctl_lock);
	_msgbuf_write_lock_ctl(msgbuf, lock);
	mutex_unlock(&msgbuf->lock_ctl_lock);
}

/**
 * @brief lock control set mask
 */
void msgbuf_set_lock_ctl_mask(struct msgbuf *msgbuf,
	const struct acmdrv_msgbuf_lock_ctrl *mask)
{
	struct acmdrv_msgbuf_lock_ctrl lock;

	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&lock);

	mutex_lock(&msgbuf->lock_ctl_lock);

	_msgbuf_read_lock_ctl(msgbuf, &lock);
	_msgbuf_write_lock_ctl(msgbuf,
		ACMDRV_MSGBUF_LOCK_CTRL_OR(&lock, &lock, mask));

	mutex_unlock(&msgbuf->lock_ctl_lock);
}

/**
 * @brief lock control unset mask
 */
void msgbuf_unset_lock_ctl_mask(struct msgbuf *msgbuf,
	const struct acmdrv_msgbuf_lock_ctrl *mask)
{
	struct acmdrv_msgbuf_lock_ctrl lock, nmask;

	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&lock);
	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&nmask);

	mutex_lock(&msgbuf->lock_ctl_lock);

	_msgbuf_read_lock_ctl(msgbuf, &lock);
	_msgbuf_write_lock_ctl(msgbuf,
		ACMDRV_MSGBUF_LOCK_CTRL_AND(&lock, &lock,
			ACMDRV_MSGBUF_LOCK_CTRL_NOT(&nmask, mask)));

	mutex_unlock(&msgbuf->lock_ctl_lock);
}

/**
 * @brief Get access mask for lock control access
 */
const
struct acmdrv_msgbuf_lock_ctrl *msgbuf_get_lock_ctrl_mask(struct msgbuf *msgbuf)
{
	return &msgbuf->mask;
}

/**
 * @brief read message buffer decriptor
 */
int __must_check msgbuf_desc_read(struct msgbuf *msgbuf, unsigned int first,
				  unsigned int last, msgbuf_desc_t *buf)
{
	int i;
	unsigned int nr = commreg_read_msgbuf_count(msgbuf->acm->commreg);

	if (first > last)
		return -EINVAL;

	if (first >= nr)
		return -EINVAL;

	if (last >= nr)
		return -EINVAL;

	for (i = first; i <= last; ++i)
		*buf++ = msgbuf->desc_cache[i];

	return 0;
}

/**
 * @brief write message buffer decriptor
 */
int __must_check msgbuf_desc_write(struct msgbuf *msgbuf, unsigned int first,
				   unsigned int last, msgbuf_desc_t *buf)
{
	int i;
	unsigned int nr = commreg_read_msgbuf_count(msgbuf->acm->commreg);

	if (first > last)
		return -EINVAL;

	if (first >= nr)
		return -EINVAL;

	if (last >= nr)
		return -EINVAL;

	for (i = first; i <= last; ++i)
		_msgbuf_write_desc(msgbuf, i, *buf++);

	return 0;
}

/**
 * @brief get message buffer size in bytes
 */
size_t msgbuf_size(const struct msgbuf *msgbuf, int i)
{
	size_t size;

	size = read_bitmask(&msgbuf->desc_cache[i],
			    ACM_MSGBUF_DESC_SUBBUFFER_SIZE) + 1;
	size *= sizeof(u32);

	return size;
}

/**
 * @brief get message buffer type
 */
enum acm_msgbuf_type msgbuf_type(const struct msgbuf *msgbuf, int i)
{
	return read_bitmask(&msgbuf->desc_cache[i], ACM_MSGBUF_DESC_TYPE);
}

/**
 * @brief check if message buffer is empty
 */
bool msgbuf_is_empty(struct msgbuf *msgbuf, int i)
{
	return ACM_MSGBUF_STATUS_EMPTY ==
		(msgbuf_read_status(msgbuf, i) & ACM_MSGBUF_STATUS_EMPTY);
}

/**
 * @brief check if message buffer is valid
 */
bool msgbuf_is_valid(const struct msgbuf *msgbuf, int i)
{
	return ACM_MSGBUF_DESC_VALID ==
		(msgbuf->desc_cache[i] & ACM_MSGBUF_DESC_VALID);
}

/**
 * @brief read message buffer data to user space
 */
int __must_check msgbuf_read_to_user(struct msgbuf *msgbuf, int i,
				     char __user *to, size_t size)
{
	int ret;
	const size_t msize = msgbuf_size(msgbuf, i);
	const enum acm_msgbuf_type type = msgbuf_type(msgbuf, i);
	const unsigned int buffers =
		commreg_read_msgbuf_count(msgbuf->acm->commreg);
	/*
	 * use u32 aligned bounce buffer to to ensure dword alignment
	 * for acm_memcpy32;
	 */
	u8 bounce[msize] __aligned(sizeof(u32));

	if (type != ACM_MSGBUF_TYPE_RX)
		return -EIO;

	if (i >= buffers)
		return -EINVAL;

	if (size > msize)
		size = msize;

	ret = mutex_lock_interruptible(&msgbuf->msgbuf_lock);
	if (ret)
		return ret;

	if (msgbuf_is_empty(msgbuf, i)) {
		mutex_unlock(&msgbuf->msgbuf_lock);
		return -ENODATA;
	}

	acm_ioread32_copy(bounce, msgbuf->base + ACM_MSGBUF_DATA(i), msize);
	mutex_unlock(&msgbuf->msgbuf_lock);

	return copy_to_user(to, bounce, size) ? -EFAULT : 0;
}

/**
 * @brief write message buffer data from user space
 */
int __must_check msgbuf_write_from_user(struct msgbuf *msgbuf, int i,
					const char __user *from, size_t size)
{
	int ret;
	const size_t msize = msgbuf_size(msgbuf, i);
	const enum acm_msgbuf_type type = msgbuf_type(msgbuf, i);
	const unsigned int buffers =
		commreg_read_msgbuf_count(msgbuf->acm->commreg);
	/*
	 * use u32 aligned bounce buffer to to ensure dword alignment
	 * for acm_memcpy32;
	 */
	u8 bounce[msize] __aligned(sizeof(u32));

	if (type != ACM_MSGBUF_TYPE_TX)
		return -EIO;

	if (size > msize)
		size = msize;

	if (i >= buffers)
		return -EINVAL;

	memset(bounce + size, 0, msize - size);

	if (copy_from_user(bounce, from, size))
		return -EFAULT;

	ret = mutex_lock_interruptible(&msgbuf->msgbuf_lock);
	if (ret)
		return ret;

	acm_iowrite32_copy(msgbuf->base + ACM_MSGBUF_DATA(i), bounce, msize);
	/* update overwritten by reading status */
	msgbuf_read_status(msgbuf, i);

	mutex_unlock(&msgbuf->msgbuf_lock);

	return 0;
}


/**
 * @brief cleanup/initialize message buffer hardware
 */
void msgbuf_cleanup(struct msgbuf *msgbuf)
{
	int i;
	unsigned int buffers = commreg_read_msgbuf_count(msgbuf->acm->commreg);

	/* clear message buffer descriptors */
	for (i = 0; i < buffers; i++)
		_msgbuf_write_desc(msgbuf, i, 0);
}

/**
 * @brief initialize message buffer handler
 */
int __must_check msgbuf_init(struct acm *acm)
{
	int i;
	struct resource *res;
	struct msgbuf *msgbuf;
	struct platform_device *pdev = acm->pdev;
	struct device *dev = &pdev->dev;
	unsigned int buffers = commreg_read_msgbuf_count(acm->commreg);

	msgbuf = devm_kzalloc(dev, sizeof(*msgbuf), GFP_KERNEL);
	if (!msgbuf)
		return -ENOMEM;
	msgbuf->acm = acm;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					   "Messagebuffer");
	msgbuf->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(msgbuf->base))
		return PTR_ERR(msgbuf->base);
	msgbuf->size = resource_size(res);

	msgbuf->overwritten = devm_kcalloc(dev, buffers,
					   sizeof(*msgbuf->overwritten),
					   GFP_KERNEL);
	if (!msgbuf->overwritten)
		return -ENOMEM;
	for (i = 0; i < buffers; ++i)
		atomic_set(&msgbuf->overwritten[i], 0);

	msgbuf->desc_cache = devm_kcalloc(dev, buffers,
					  sizeof(*msgbuf->desc_cache),
					  GFP_KERNEL);
	if (!msgbuf->desc_cache)
		return -ENOMEM;

	mutex_init(&msgbuf->lock_ctl_lock);
	mutex_init(&msgbuf->desc_lock);
	mutex_init(&msgbuf->msgbuf_lock);
	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&msgbuf->mask);

	/* prefill cache and lock control mask */
	for (i = 0; i < buffers; ++i) {
		ACMDRV_MSGBUF_LOCK_CTRL_SET(i, &msgbuf->mask);
		_msgbuf_read_desc(msgbuf, i);
	}

	dev_dbg(dev, "Probed %u Messagebuffers %pr -> 0x%p\n", buffers, res,
		msgbuf->base);

	acm->msgbuf = msgbuf;
	return 0;
}

/**
 * @brief exit message buffer handler
 */
void msgbuf_exit(struct acm *acm)
{
	/* nothing to do */
}
/**@} hwaccmsgbuf */
