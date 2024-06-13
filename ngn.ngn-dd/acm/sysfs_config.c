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
 * @file sysfs_config.c
 * @brief ACM Driver SYSFS - Config Section
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup acmsysfsconfigimpl ACM SYSFS Config Implementation
 * @brief Implementation of SYSFS Config Section
 *
 * @{
 */

#include <linux/sysfs.h>
#include <asm/unaligned.h>

#include "acm-module.h"
#include "state.h"
#include "chardev.h"
#include "sysfs_config.h"
#include "msgbuf.h"
#include "bypass.h"
#include "redundancy.h"
#include "scheduler.h"
#include "config.h"
#include "sysfs.h"
#include "commreg.h"

/**
 * @brief Extended binary device attribute for bypass attributes
 *
 * It eases to translate the array-like sysfs interface to the various
 * distributes of the information within the ACM IP.
 */
struct sysfs_bypass_attr {
	size_t elemsize;	/**< @brief Size of a single item */
	size_t padsize;		/**< @brief Size of a single item on sysfs */
	loff_t offset;		/**< @brief Address offset of the first item */
	loff_t delta;		/**< @brief Address delta  between two items */
	struct mutex lock;	/**< @brief Access lock */

	struct bin_attribute bin_attr; /**< @brief Linux bin_attr base struct */
};

/**
 * @def ACM_SYSFS_BYPASS_ATTR_RW
 * @brief Static initializer helper macro for struct sysfs_bypass_attr data
 *
 * @param _name base name of the attribute
 * @param _elsize size of a single item of the attribute
 * @param _elno total number of items within the attribute
 * @param _offs register offset of the first item within a bypass module
 * @param _delta interval between two items within the ACM IP
 */
#define ACM_SYSFS_BYPASS_ATTR_RW(_name, _elsize, _elno, _offs, _delta)	\
struct sysfs_bypass_attr sysfs_bypass_attr_##_name =			\
{									\
	.elemsize	= (_elsize),					\
	.padsize	= (_elsize),					\
	.offset		= (_offs),					\
	.delta		= (_delta),					\
	.lock		= __MUTEX_INITIALIZER(				\
				sysfs_bypass_attr_##_name.lock),	\
	.bin_attr	= __BIN_ATTR(_name, 0644,			\
				     sysfs_bypass_attr_read,		\
				     sysfs_bypass_attr_write,		\
				     (_elsize) * (_elno)),		\
}

/**
 * @def ACM_SYSFS_BYPASS_ATTR_WO
 * @brief Static initializer helper macro for struct sysfs_bypass_attr data
 *
 * @param _name base name of the attribute
 * @param _elsize size of a single item of the attribute
 * @param _elno total number of items within the attribute
 * @param _offs register offset of the first item within a bypass module
 * @param _delta interval between two items within the ACM IP
 */
#define ACM_SYSFS_BYPASS_ATTR_WO(_name, _elsize, _elno, _offs, _delta)  \
struct sysfs_bypass_attr sysfs_bypass_attr_##_name =			\
{									\
	.elemsize	= (_elsize),					\
	.padsize	= (_elsize),					\
	.offset		= (_offs),					\
	.delta		= (_delta),					\
	.lock		= __MUTEX_INITIALIZER(				\
				sysfs_bypass_attr_##_name.lock),	\
	.bin_attr	= __BIN_ATTR(_name, 0200,			\
				sysfs_bypass_attr_read,			\
				sysfs_bypass_attr_write,		\
				(_elsize) * (_elno)),			\
}

/**
 * @def ACM_SYSFS_BYPASS_ATTR_RW_PADDED
 * @brief Static initializer helper macro for padded struct sysfs_bypass_attr
 *        data
 *
 * @param _name Base name of the attribute
 * @param _elsize Size of a single item of the attribute
 * @param _elno Total number of items within the attribute
 * @param _offs Register offset of the first item within a bypass module
 * @param _delta Interval between two items within the ACM IP
 * @param _padsize Size of an item inclusively padding at the sysfs interface
 */
#define ACM_SYSFS_BYPASS_ATTR_RW_PADDED(_name, _elsize, _elno, _offs,	\
					_delta,  _padsize)		\
struct sysfs_bypass_attr sysfs_bypass_attr_##_name =			\
{									\
	.elemsize	= (_elsize),					\
	.padsize	= (_padsize),					\
	.offset		= (_offs),					\
	.delta		= (_delta),					\
	.lock		= __MUTEX_INITIALIZER(				\
				sysfs_bypass_attr_##_name.lock),	\
	.bin_attr	= __BIN_ATTR(_name, 0644,			\
				     sysfs_bypass_attr_read,		\
				     sysfs_bypass_attr_write,		\
				     (_padsize) * (_elno)),		\
}

/**
 * @def ACM_SYSFS_BYPASS_ATTR_WO_PADDED
 * @brief Static initializer helper macro for padded struct sysfs_bypass_attr
 *        data
 *
 * @param _name Base name of the attribute
 * @param _elsize Size of a single item of the attribute
 * @param _elno Total number of items within the attribute
 * @param _offs Register offset of the first item within a bypass module
 * @param _delta Interval between two items within the ACM IP
 * @param _padsize Size of an item inclusively padding at the sysfs interface
 */
#define ACM_SYSFS_BYPASS_ATTR_WO_PADDED(_name, _elsize, _elno, _offs,	\
					_delta,  _padsize)		\
struct sysfs_bypass_attr sysfs_bypass_attr_##_name =			\
{									\
	.elemsize	= (_elsize),					\
	.padsize	= (_padsize),					\
	.offset		= (_offs),					\
	.delta		= (_delta),					\
	.lock		= __MUTEX_INITIALIZER(				\
				sysfs_bypass_attr_##_name.lock),	\
	.bin_attr	= __BIN_ATTR(_name, 0200,			\
				     sysfs_bypass_attr_read,		\
				     sysfs_bypass_attr_write,		\
				     (_padsize) * (_elno)),		\
}

/**
 * @brief read function for struct sysfs_bypass_attr
 *
 * Generic function that reads the respective bypass module data into a sysfs
 * array.
 *
 * @param file Standard parameter not used in here
 * @param kobj Kernel object the attribute belongs to
 * @param bin_attr binary attribute given
 * @param buf Data buffer to read into
 * @param off Offset where to read the data from within the binary attribute
 * @param size Amount of data to read
 * @return Return number of bytes read into the buffer or negative error id
 */
static ssize_t sysfs_bypass_attr_read(struct file *file, struct kobject *kobj,
				      struct bin_attribute *bin_attr, char *buf,
				      loff_t off, size_t size)
{
	int ret;
	unsigned int i, nr_items;
	struct acm *acm = kobj_to_acm(kobj);
	/* use short offset to avoid 64bit division */
	struct sysfs_bypass_attr *attr =
		container_of(bin_attr, struct sysfs_bypass_attr,
			     bin_attr);

	ret = sysfs_bin_attr_check(bin_attr, off, size, attr->padsize);
	if (ret)
		return ret;

	/* number of items per bypass module */
	nr_items = bin_attr->size / attr->padsize / ACMDRV_BYPASS_MODULES_COUNT;

	ret = mutex_lock_interruptible(&attr->lock);
	if (ret)
		return ret;
	foreach_item(i, off, size, attr->padsize) {
		struct bypass *bp = acm->bypass[i / nr_items];

		bypass_block_read(bp, buf,
				  attr->offset + (i % nr_items) * attr->delta,
				  attr->elemsize);
		buf += attr->padsize;
	}
	mutex_unlock(&attr->lock);

	return size;
}

/**
 * @brief write function for struct sysfs_bypass_attr
 *
 * Generic function that writes the respective bypass module data from a sysfs
 * array.
 *
 * @param file Standard parameter not used in here
 * @param kobj Kernel object the attribute belongs to
 * @param bin_attr Binary attribute given
 * @param buf Data buffer to read from
 * @param off Offset where to write the data from within the binary attribute
 * @param size Amount of data to write
 * @return Return number of bytes written or negative error id
 */
static ssize_t sysfs_bypass_attr_write(struct file *file, struct kobject *kobj,
				       struct bin_attribute *bin_attr,
				       char *buf, loff_t off, size_t size)
{
	int ret;
	unsigned int i, nr_items;
	struct acm *acm = kobj_to_acm(kobj);
	/* use short offset to avoid 64bit division */
	struct sysfs_bypass_attr *attr =
		container_of(bin_attr, struct sysfs_bypass_attr,
			     bin_attr);

	ret = sysfs_bin_attr_check(bin_attr, off, size, attr->padsize);
	if (ret)
		return ret;

	/* number of items per bypass module */
	nr_items = bin_attr->size / attr->padsize / ACMDRV_BYPASS_MODULES_COUNT;

	ret = mutex_lock_interruptible(&attr->lock);
	if (ret)
		return ret;

	foreach_item(i, off, size, attr->padsize) {
		struct bypass *bp = acm->bypass[i / nr_items];

		bypass_block_write(
			bp, buf, attr->offset + (i % nr_items) * attr->delta,
			attr->elemsize);

		buf += attr->padsize;
	}
	mutex_unlock(&attr->lock);

	return size;
}



/**
 * @brief extended binary device attribute for bypass register access
 *
 * Thus generalizes the access to register based attributes of a bypass module.
 */
struct sysfs_bypass_reg_attr {
	off_t area;		/**< @brief Submodule offset of first item */
	off_t offset;		/**< @brief Address offset of the first item */
	struct mutex lock;	/**< @brief Access lock */

	struct bin_attribute bin_attr;	/**< @brief Linux bin_attribute base */
};

/**
 * @def ACM_SYSFS_BYPASS_REG_ATTR_RW
 * @brief Static initializer helper macro for struct sysfs_bypass_reg_attr
 *
 * @param _name Base name of the attribute
 * @param _area Subsection offset with a bypass module
 * @param _offs Register offset of the respective register within the subsection
 */
#define ACM_SYSFS_BYPASS_REG_ATTR_RW(_name, _area, _offs)		\
struct sysfs_bypass_reg_attr sysfs_bypass_reg_attr_##_name =		\
{									\
	.area		= (_area),					\
	.offset		= (_offs),					\
	.lock		= __MUTEX_INITIALIZER(				\
				sysfs_bypass_reg_attr_##_name.lock),	\
	.bin_attr	= __BIN_ATTR(_name, 0644,			\
				sysfs_bypass_reg_read,			\
				sysfs_bypass_reg_write,			\
				sizeof(u32) * ACMDRV_BYPASS_MODULES_COUNT) \
}

/**
 * @brief read function for struct sysfs_bypass_reg_attr
 *
 * Generic function that reads the respective bypass module registers into a
 * sysfs array.
 *
 * @param file Standard parameter not used in here
 * @param kobj Kernel object the attribute belongs to
 * @param bin_attr Binary attribute given
 * @param buf Data buffer to read into
 * @param off Offset where to read the data from within the binary attribute
 * @param size Amount of data to read
 * @return Return number of bytes read into the buffer or negative error id
 */
static ssize_t sysfs_bypass_reg_read(struct file *file, struct kobject *kobj,
				     struct bin_attribute *bin_attr, char *buf,
				     loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	/* use short offset to avoid 64bit division */
	struct sysfs_bypass_reg_attr *attr =
		container_of(bin_attr, struct sysfs_bypass_reg_attr,
			     bin_attr);

	ret = sysfs_bin_attr_check(bin_attr, off, size, sizeof(u32));
	if (ret)
		return ret;

	ret = mutex_lock_interruptible(&attr->lock);
	if (ret)
		return ret;
	foreach_item(i, off, size, sizeof(u32)) {
		u32 reg = bypass_area_read(acm->bypass[i], attr->area,
					   attr->offset);

		put_unaligned(reg, (u32 *)buf);
		buf += sizeof(u32);
	}
	mutex_unlock(&attr->lock);
	return size;
}

/**
 * @brief write function for struct sysfs_bypass_reg_attr
 *
 * Generic function that writes the respective bypass module registers from a
 * sysfs array.
 *
 * @param file Standard parameter not used in here
 * @param kobj Kernel object the attribute belongs to
 * @param bin_attr Binary attribute given
 * @param buf Data buffer to read from
 * @param off Offset where to write the data from within the binary attribute
 * @param size Amount of data to write
 * @return Return number of bytes written or negative error id
 */
static ssize_t sysfs_bypass_reg_write(struct file *file, struct kobject *kobj,
				      struct bin_attribute *bin_attr, char *buf,
				      loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	/* use short offset to avoid 64bit division */
	struct sysfs_bypass_reg_attr *attr =
		container_of(bin_attr, struct sysfs_bypass_reg_attr,
			     bin_attr);

	ret = sysfs_bin_attr_check(bin_attr, off, size, sizeof(u32));
	if (ret)
		return ret;

	ret = mutex_lock_interruptible(&attr->lock);
	if (ret)
		return ret;
	foreach_item(i, off, size, sizeof(u32)) {
		u32 reg = get_unaligned((u32 *)buf);

		bypass_area_write(acm->bypass[i], attr->area,
				  attr->offset, reg);

		buf += sizeof(u32);
	}
	mutex_unlock(&attr->lock);
	return size;
}

/**
 * @brief Read the state of the ACM-device driver.
 *
 * @param file Standard parameter not used in here
 * @param kobj Kernel object the attribute belongs to
 * @param bin_attr Binary attribute given
 * @param buf Data buffer to read into
 * @param off Offset where to read the data from within the binary attribute
 * @param size Amount of data to read
 * @return Return number of bytes read into the buffer or negative error id
 */
static ssize_t config_state_read(struct file *file, struct kobject *kobj,
				 struct bin_attribute *bin_attr, char *buf,
				 loff_t off, size_t size)
{
	int ret;
	struct acm *acm = kobj_to_acm(kobj);
	enum acmdrv_status status = acm_state_get(acm->status);

	ret = sysfs_bin_attr_check(bin_attr, off, size,
				   sizeof(enum acmdrv_status));
	if (ret)
		return ret;

	memcpy(buf, ((u8 *)&status) + off, size);
	return size;
}

/**
 * @brief Write the state of the ACM-device driver.
 *
 * @param file Standard parameter not used in here
 * @param kobj Kernel object the attribute belongs to
 * @param bin_attr Binary attribute given
 * @param buf Data buffer to read from
 * @param off Offset where to write the data from within the binary attribute
 * @param size Amount of data to write
 * @return Return number of bytes written or negative error id
 */
static ssize_t config_state_write(struct file *file, struct kobject *kobj,
				  struct bin_attribute *bin_attr, char *buf,
				  loff_t off, size_t size)
{
	int ret;
	enum acmdrv_status status;
	struct acm *acm = kobj_to_acm(kobj);

	ret = sysfs_bin_attr_check(bin_attr, off, size,
				   sizeof(enum acmdrv_status));
	if (ret)
		return ret;

	status = *((enum acmdrv_status *)buf);
	ret = acm_state_set(acm->status, status);
	if (ret)
		return ret;

	return size;
}

/**
 * @brief Config attribute config_state
 */
static BIN_ATTR_RW(config_state, sizeof(enum acmdrv_status));

/**
 * @brief Read the message buffer descriptor(s)
 *
 * The first and last descriptors read are determined by off and count which
 * must be aligned to sizeof(msgbuf_desc_t). Furthermore the otherwise
 * unused reserv2 bit (Bit 30) is used to indicate time-stamping flag of the
 * corresponding message buffer character device.
 *
 * @param file Standard parameter not used in here
 * @param kobj kernel object the attribute belongs to
 * @param bin_attr binary attribute given
 * @param buf Data buffer to read into
 * @param off offset where to read the data from within the binary attribute
 * @param size amount of data to read
 * @return return number of bytes read into the buffer or negative error id
 */
static ssize_t msg_buff_desc_read(struct file *file, struct kobject *kobj,
				  struct bin_attribute *bin_attr, char *buf,
				  loff_t off, size_t size)
{
	int ret, i;
	unsigned int first, last;
	struct acm *acm = kobj_to_acm(kobj);
	const unsigned int buffers = commreg_read_msgbuf_count(acm->commreg);
	const size_t elsize = sizeof(msgbuf_desc_t);
	msgbuf_desc_t bounce[buffers];

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	first = off / elsize;
	last = (off + size) / elsize - 1;

	ret = msgbuf_desc_read(acm->msgbuf, first, last, &bounce[first]);
	if (ret)
		return ret;

	for (i = first; i <= last; ++i) {
		msgbuf_desc_t *desc = &bounce[i];
		struct acm_dev *adev = ACM_DEVICE(acm, i);

		*desc &= ~ACMDRV_BUFF_DESC_HAS_TIMESTAMP;
		*desc |= acm_dev_get_timestamping(adev) ?
			ACMDRV_BUFF_DESC_HAS_TIMESTAMP : 0;
	}
	memcpy(buf, ((u8 *)&bounce[first]), size);

	return size;
}

/**
 * @brief Write message buffer descriptor(s)
 *
 * The first and last descriptors written are determined by off and count which
 * must be aligned to sizeof(msgbuf_desc_t). Furthermore the otherwise
 * unused reserv2 bit (Bit 30) is used to set/unset the time-stamping flag of
 * the corresponding message buffer character device.
 *
 * @param file Standard parameter not used in here
 * @param kobj kernel object the attribute belongs to
 * @param bin_attr binary attribute given
 * @param buf Data buffer to read from
 * @param off offset where to write the data from within the binary attribute
 * @param size amount of data to write
 * @return return number of bytes written or negative error id
 */
static ssize_t msg_buff_desc_write(struct file *file, struct kobject *kobj,
				   struct bin_attribute *bin_attr, char *buf,
				   loff_t off, size_t size)
{
	int ret, i;
	unsigned int first, last;
	struct acm *acm = kobj_to_acm(kobj);
	const unsigned int buffers = commreg_read_msgbuf_count(acm->commreg);
	const size_t elsize = sizeof(msgbuf_desc_t);
	msgbuf_desc_t bounce[buffers];

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	first = off / elsize;
	last = (off + size) / elsize - 1;

	memcpy(((u8 *)&bounce[first]), buf, size);
	for (i = first; i <= last; ++i) {
		msgbuf_desc_t *desc = &bounce[i];
		struct acm_dev *adev = ACM_DEVICE(acm, i);

		if (*desc & ACMDRV_BUFF_DESC_HAS_TIMESTAMP)
			acm_dev_set_timestamping(adev, true);
		else
			acm_dev_set_timestamping(adev, false);

		*desc &= ~ACMDRV_BUFF_DESC_HAS_TIMESTAMP;
	}
	ret = msgbuf_desc_write(acm->msgbuf, first, last, &bounce[first]);
	if (ret)
		return ret;
	return size;
}

/**
 * @brief Config attribute msg_buff_desc
 */
static BIN_ATTR_RW(msg_buff_desc, 0 /* size set by init */);

/**
 * @brief Read the message buffer alias(es) in a row
 *
 * The first and last descriptors read are determined by off and count which
 * must be aligned to sizeof(struct acm_msgbuf_alias). All message buffer
 * aliases must be configured, so you cannot examine an unconfigured alias.
 *
 * @param filp unused standard parameter
 * @param kobj associated kernel object pointer
 * @param bin_attr unused standard parameter
 * @param buf data buffer to read into.
 * @param off data offset to the first descriptor. Must be a multiple of
 *            sizeof(struct acm_msgbuf_alias)
 * @param size number of bytes to read. Must be a multiple of
 *              sizeof(struct acm_msgbuf_alias)
 * @return return number of bytes read successfully, or -(error-id) if an error
 *         occurred.
 */
static ssize_t msg_buff_alias_read(struct file *filp, struct kobject *kobj,
				   struct bin_attribute *bin_attr, char *buf,
				   loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(struct acmdrv_buff_alias);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	/* ensure all ACM devices are active */
	foreach_item(i, off, size, elsize) {
		struct acm_dev *adev = ACM_DEVICE(acm, i);

		if (!acm_dev_is_active(adev))
			/*
			 * TODO: should we use -ENODEV instead?
			 * This is a compatibility sacrifice to the old
			 * driver's behavior. Check if any application
			 * relies on this behavior!
			 */
			return 0;
	}

	foreach_item(i, off, size, elsize) {
		struct acmdrv_buff_alias *alias =
			(struct acmdrv_buff_alias *)buf;
		struct acm_dev *adev = ACM_DEVICE(acm, i);
		const char *name;

		alias->idx = i;
		alias->id = acm_dev_get_id(adev);
		name = acm_dev_get_name(adev);
		strncpy(alias->alias, name, sizeof(alias->alias) - 1);
		alias->alias[sizeof(alias->alias) - 1] = 0;
		buf += elsize;
	}

	return size;
}

/**
 * @brief Write message buffer alias(es)
 *
 * This writes an array of message buffer aliases (message buffer according to
 * alias data) and thus creates, configures and activates the corresponding
 * ACM devices for message buffer data transfer,
 *
 * @param filp unused standard parameter
 * @param kobj associated kernel object pointer
 * @param bin_attr unused standard parameter
 * @param buf data buffer to write from (message buffer alias array)
 * @param off Must be 0 TODO ???
 * @param size number of bytes to write. Must be a multiple of
 *              sizeof(struct acm_msgbuf_alias)
 * @return return number of bytes read successfully, or -(error-id) if an error
 *         occurred.
 */
static ssize_t msg_buff_alias_write(struct file *filp, struct kobject *kobj,
				    struct bin_attribute *bin_attr, char *buf,
				    loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const unsigned int buffers = commreg_read_msgbuf_count(acm->commreg);
	const size_t elsize = sizeof(struct acmdrv_buff_alias);
	char *lbuf = buf;

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	/* check if all aliases are in range and not yet configured */
	foreach_item(i, off, size, elsize) {
		struct acmdrv_buff_alias *alias =
			(struct acmdrv_buff_alias *)lbuf;

		if (alias->idx >= buffers) {
			dev_warn(&acm->dev, "%s: index out of range",
				 __func__);
			return -EINVAL;
		}

		if (alias->alias[0] == 0) {
			dev_warn(&acm->dev, "%s: no alias name given",
				 __func__);
			return -EINVAL;
		}

		if (i != alias->idx) {
			dev_warn(&acm->dev, "%s: index mismatch error: %d != %d",
				 __func__, i, alias->idx);
			return -EINVAL;
		}

		if (acm_dev_is_active(ACM_DEVICE(acm, alias->idx))) {
			dev_warn(&acm->dev, "%s: device %d in use",
				 __func__, i);
			return -EBUSY;
		}

		lbuf += elsize;
	}

	lbuf = buf;
	foreach_item(i, off, size, elsize) {
		struct acmdrv_buff_alias *alias =
			(struct acmdrv_buff_alias *)lbuf;
		struct acm_dev *adev = ACM_DEVICE(acm, i);

		ret = acm_dev_activate(adev, alias->alias, alias->id);
		if (ret)
			return ret;
		lbuf += elsize;
	}

	return size;
}

/**
 * @brief Config attribute msg_buff_alias
 */
static BIN_ATTR_RW(msg_buff_alias, 0 /* size set by init */);

/**
 * @brief read function for redund_cnt_tab
 */
static ssize_t redund_cnt_tab_read(struct file *filp, struct kobject *kobj,
				   struct bin_attribute *bin_attr, char *buf,
				   loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	const size_t elsize = sizeof(struct acmdrv_redun_ctrl_entry);
	struct acm *acm = kobj_to_acm(kobj);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		int tab, idx;
		u32 entry;

		tab = i / ACMDRV_REDUN_TABLE_ENTRY_COUNT;
		idx = i % ACMDRV_REDUN_TABLE_ENTRY_COUNT;

		entry = redundancy_area_read(acm->redundancy,
					      ACM_REDUN_CTRLTAB(tab),
					      idx * elsize);
		put_unaligned(entry, (u32 *)buf);
		buf += elsize;
	}
	return size;
}

/**
 * @brief write function for redund_cnt_tab
 */
static ssize_t redund_cnt_tab_write(struct file *filp, struct kobject *kobj,
				    struct bin_attribute *bin_attr, char *buf,
				    loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	const size_t elsize = sizeof(struct acmdrv_redun_ctrl_entry);
	struct acm *acm = kobj_to_acm(kobj);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		int tab, idx;
		u32 entry;

		tab = i / ACMDRV_REDUN_TABLE_ENTRY_COUNT;
		idx = i % ACMDRV_REDUN_TABLE_ENTRY_COUNT;

		entry = get_unaligned((u32 *)buf);
		redundancy_area_write(acm->redundancy,
				      ACM_REDUN_CTRLTAB(tab),
				      idx * elsize, entry);
		buf += elsize;
	}
	return size;
}

/**
 * @brief Config attribute redund_cnt_tab
 */
static BIN_ATTR_RW(redund_cnt_tab,
		   ACMDRV_BYPASS_MODULES_COUNT *
		   ACMDRV_REDUN_TABLE_ENTRY_COUNT *
		   sizeof(struct acmdrv_redun_ctrl_entry));

/**
 * @brief Attribute read function for redund_status_tab
 */
static ssize_t redund_status_tab_read(struct file *filp, struct kobject *kobj,
				      struct bin_attribute *bin_attr, char *buf,
				      loff_t off, size_t count)
{
	int ret;
	const size_t elsize = sizeof(struct acmdrv_redun_status);
	struct acm *acm = kobj_to_acm(kobj);

	ret = sysfs_bin_attr_check(bin_attr, off, count, elsize);
	if (ret)
		return ret;

	redundancy_block_read(acm->redundancy, buf,
			      ACM_REDUN_STATUSTAB + off, count);

	return count;
}

/**
 * @brief Config attribute redund_status_tab
 */
static BIN_ATTR_RO(redund_status_tab,
		   ACMDRV_REDUN_TABLE_ENTRY_COUNT *
		   sizeof(struct acmdrv_redun_status));

/**
 * @brief Attribute read function for intseqnum_tab
 */
static ssize_t redund_intseqnum_tab_read(struct file *filp,
					 struct kobject *kobj,
					 struct bin_attribute *bin_attr,
					 char *buf, loff_t off, size_t count)
{
	int ret;
	const size_t elsize = sizeof(struct acmdrv_redun_intseqnum);
	struct acm *acm = kobj_to_acm(kobj);

	ret = sysfs_bin_attr_check(bin_attr, off, count, elsize);
	if (ret)
		return ret;

	redundancy_block_read(acm->redundancy, buf,
			      ACM_REDUN_INTSEQNNUMTAB + off, count);

	return count;
}

/**
 * @brief Attribute write function for intseqnum_tab
 */
static ssize_t redund_intseqnum_tab_write(struct file *filp,
					  struct kobject *kobj,
					  struct bin_attribute *bin_attr,
					  char *buf, loff_t off, size_t count)
{
	int ret;
	const size_t elsize = sizeof(struct acmdrv_redun_intseqnum);
	struct acm *acm = kobj_to_acm(kobj);

	ret = sysfs_bin_attr_check(bin_attr, off, count, elsize);
	if (ret)
		return ret;

	redundancy_block_write(acm->redundancy, buf,
			       ACM_REDUN_INTSEQNNUMTAB + off, count);
	return count;
}

/**
 * @brief Config attribute redund_intseqnum_tab
 */
static BIN_ATTR_RW(redund_intseqnum_tab,
		   ACMDRV_REDUN_TABLE_ENTRY_COUNT *
		   sizeof(struct acmdrv_redun_intseqnum));

/**
 * @brief read function for sched_down_counter
 */
static ssize_t sched_down_counter_read(struct file *filp, struct kobject *kobj,
				       struct bin_attribute *bin_attr,
				       char *buf, loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(struct acmdrv_scheduler_down_counter);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		struct acmdrv_scheduler_down_counter cnt;

		cnt.counter = scheduler_read_sched_down_counter(acm->scheduler,
								i);
		put_unaligned(cnt.counter, (u16 *)buf);
		buf += elsize;
	}
	return size;
}

/**
 * @brief write function for sched_down_counter
 */
static ssize_t sched_down_counter_write(struct file *filp, struct kobject *kobj,
					struct bin_attribute *bin_attr,
					char *buf, loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(struct acmdrv_scheduler_down_counter);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		struct acmdrv_scheduler_down_counter cnt;

		cnt.counter = get_unaligned((u16 *)buf);
		scheduler_write_sched_down_counter(acm->scheduler, i,
						   cnt.counter);

		buf += elsize;
	}
	return size;
}

/**
 * @brief Config attribute sched_down_counter
 */
static BIN_ATTR_RW(sched_down_counter,
		   ACMDRV_SCHEDULER_COUNT *
		   sizeof(struct acmdrv_scheduler_down_counter));

/**
 * @brief read function for sched_tab_row
 */
static ssize_t sched_tab_row_read(struct file *filp, struct kobject *kobj,
				  struct bin_attribute *bin_attr, char *buf,
				  loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(struct acmdrv_sched_tbl_row);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		int sched_id, tab_id, row_id;
		struct acmdrv_sched_tbl_row row = { 0 };

		sched_id = (i / ACMDRV_SCHED_TBL_ROW_COUNT) /
			ACMDRV_SCHED_TBL_COUNT;
		tab_id = (i / ACMDRV_SCHED_TBL_ROW_COUNT) %
			ACMDRV_SCHED_TBL_COUNT;
		row_id = i % ACMDRV_SCHED_TBL_ROW_COUNT;

		ret = scheduler_read_table_row(acm->scheduler, sched_id, tab_id,
					       row_id, &row);
		if (ret)
			return ret;

		memcpy(buf, &row, elsize);
		buf += elsize;
	}
	return size;
}

/**
 * @brief write function for sched_tab_row
 */
static ssize_t sched_tab_row_write(struct file *filp, struct kobject *kobj,
				   struct bin_attribute *bin_attr, char *buf,
				   loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(struct acmdrv_sched_tbl_row);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		int sched_id, tab_id, row_id;
		struct acmdrv_sched_tbl_row row = { 0 };

		sched_id = (i / ACMDRV_SCHED_TBL_ROW_COUNT) /
			ACMDRV_SCHED_TBL_COUNT;
		tab_id = (i / ACMDRV_SCHED_TBL_ROW_COUNT) %
			ACMDRV_SCHED_TBL_COUNT;
		row_id = i % ACMDRV_SCHED_TBL_ROW_COUNT;

		memcpy(&row, buf, elsize);
		ret = scheduler_write_table_row(acm->scheduler, sched_id,
						tab_id, row_id, &row);
		if (ret)
			return ret;
		buf += elsize;
	}
	return size;
}

/**
 * @brief Config attribute sched_tab_row
 */
static BIN_ATTR_RW(sched_tab_row, ACMDRV_SCHEDULER_COUNT *
		   ACMDRV_SCHED_TBL_COUNT *
		   ACMDRV_SCHED_TBL_ROW_COUNT *
		   sizeof(struct acmdrv_sched_tbl_row));

/**
 * @brief read function for table_status
 */
static ssize_t table_status_read(struct file *filp, struct kobject *kobj,
				   struct bin_attribute *bin_attr, char *buf,
				   loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(struct acmdrv_sched_tbl_status);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		int sched_id, tab_id;
		struct acmdrv_sched_tbl_status status;

		sched_id = i / ACMDRV_SCHED_TBL_COUNT;
		tab_id = i % ACMDRV_SCHED_TBL_COUNT;

		status.status = scheduler_read_table_status(acm->scheduler,
							    sched_id,
							    tab_id);
		if (ret)
			return ret;

		put_unaligned(status.status, (u16 *)buf);
		buf += elsize;
	}
	return size;
}

/**
 * @brief Config attribute table_status
 */
static BIN_ATTR_RO(table_status, ACMDRV_SCHEDULER_COUNT *
		   ACMDRV_SCHED_TBL_COUNT *
		   sizeof(struct acmdrv_sched_tbl_status));

/**
 * @brief read function for sched_cycle_time
 */
static ssize_t sched_cycle_time_read(struct file *filp, struct kobject *kobj,
				     struct bin_attribute *bin_attr, char *buf,
				     loff_t off, size_t count)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(struct acmdrv_sched_cycle_time);

	ret = sysfs_bin_attr_check(bin_attr, off, count, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, count, elsize) {
		int sched_id, tab_id;
		struct acmdrv_sched_cycle_time time;

		sched_id = i / ACMDRV_SCHED_TBL_COUNT;
		tab_id = i % ACMDRV_SCHED_TBL_COUNT;

		scheduler_read_cycle_time(acm->scheduler, sched_id, tab_id,
					  &time);
		memcpy(buf, &time, elsize);
		buf += elsize;
	}
	return count;
}

/**
 * @brief write function for sched_cycle_time
 */
static ssize_t sched_cycle_time_write(struct file *filp, struct kobject *kobj,
				      struct bin_attribute *bin_attr, char *buf,
				      loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(struct acmdrv_sched_cycle_time);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		int sched_id, tab_id;
		struct acmdrv_sched_cycle_time time;

		sched_id = i / ACMDRV_SCHED_TBL_COUNT;
		tab_id = i % ACMDRV_SCHED_TBL_COUNT;

		memcpy(&time, buf, elsize);
		scheduler_write_cycle_time(acm->scheduler, sched_id, tab_id,
					   &time);
		buf += elsize;
	}
	return size;
}

/**
 * @brief Config attribute sched_cycle_time
 */
static BIN_ATTR_RW(sched_cycle_time, ACMDRV_SCHEDULER_COUNT *
		ACMDRV_SCHED_TBL_COUNT *
		sizeof(struct acmdrv_sched_cycle_time));

/**
 * @brief read function for sched_start_table
 */
static ssize_t sched_start_table_read(struct file *filp, struct kobject *kobj,
				      struct bin_attribute *bin_attr, char *buf,
				      loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(struct acmdrv_timespec64);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		int sched_id, tab_id;
		struct timespec64 time;
		struct acmdrv_timespec64 acm_time;

		sched_id = i / ACMDRV_SCHED_TBL_COUNT;
		tab_id = i % ACMDRV_SCHED_TBL_COUNT;

		scheduler_read_start_time(acm->scheduler, sched_id, tab_id,
					  &time);
		acm_time = timespec64_to_start_time(time);
		memcpy(buf, &acm_time, elsize);
		buf += elsize;
	}
	return size;
}

/**
 * @brief write function for sched_start_table
 */
static ssize_t sched_start_table_write(struct file *filp, struct kobject *kobj,
				       struct bin_attribute *bin_attr,
				       char *buf, loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(struct acmdrv_timespec64);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		int sched_id, tab_id;
		struct timespec64 time;
		struct acmdrv_timespec64 acm_time;

		sched_id = i / ACMDRV_SCHED_TBL_COUNT;
		tab_id = i % ACMDRV_SCHED_TBL_COUNT;

		memcpy(&acm_time, buf, elsize);
		time = start_time_to_timespec64(acm_time);
		scheduler_write_start_time(acm->scheduler, sched_id, tab_id,
					   &time, true);
		buf += elsize;
	}
	return size;
}

/**
 * @brief Config attribute sched_start_table
 */
static BIN_ATTR_RW(sched_start_table, ACMDRV_SCHEDULER_COUNT *
		   ACMDRV_SCHED_TBL_COUNT *
		   sizeof(struct acmdrv_timespec64));

/**
 * @brief read function for emergency_disable
 */
static ssize_t emergency_disable_read(struct file *filp, struct kobject *kobj,
				      struct bin_attribute *bin_attr, char *buf,
				      loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(struct acmdrv_sched_emerg_disable);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		struct acmdrv_sched_emerg_disable ed;

		ed.eme_dis = scheduler_read_emergency_disable(acm->scheduler,
				i);

		put_unaligned(ed.eme_dis, (u16 *)buf);
		buf += elsize;
	}
	return size;
}

/**
 * @brief write function for emergency_disable
 */
static ssize_t emergency_disable_write(struct file *filp, struct kobject *kobj,
				       struct bin_attribute *bin_attr,
				       char *buf, loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(struct acmdrv_sched_emerg_disable);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		struct acmdrv_sched_emerg_disable ed;

		ed.eme_dis = get_unaligned((u16 *)buf);

		scheduler_write_emergency_disable(acm->scheduler, i,
						  ed.eme_dis);
		buf += elsize;
	}
	return size;
}

/**
 * @brief Config attribute emergency_disable
 */
static BIN_ATTR_RW(emergency_disable, ACMDRV_SCHEDULER_COUNT *
	sizeof(struct acmdrv_sched_emerg_disable));

/**
 * @brief read function for configuration_id
 */
static ssize_t configuration_id_read(struct file *file, struct kobject *kobj,
				     struct bin_attribute *bin_attr, char *buf,
				     loff_t off, size_t size)
{
	struct acm *acm = kobj_to_acm(kobj);

	/* only can read entire id */
	if (!buf || (off != 0) || (size != sizeof(u32)))
		return -EINVAL;

	put_unaligned(commreg_read_configuration_id(acm->commreg), (u32 *)buf);

	return size;
}

/**
 * @brief write function for configuration_id
 */
static ssize_t configuration_id_write(struct file *file, struct kobject *kobj,
				      struct bin_attribute *bin_attr, char *buf,
				      loff_t off, size_t size)
{
	struct acm *acm = kobj_to_acm(kobj);

	/* only can write entire id */
	if (!buf || (off != 0) || (size != sizeof(u32)))
		return -EINVAL;

	commreg_write_configuration_id(acm->commreg, get_unaligned((u32 *)buf));

	return size;
}

/**
 * @brief Config attribute configuration_id
 */
static BIN_ATTR_RW(configuration_id, sizeof(u32));

/**
 * @brief lookup_pattern sysfs interface
 */
static ACM_SYSFS_BYPASS_ATTR_WO(
	lookup_pattern,
	sizeof(struct acmdrv_bypass_lookup),
	ACMDRV_BYPASS_MODULES_COUNT * ACMDRV_BYPASS_NR_RULES,
	ACM_BYPASS_STREAM_IDENT_LOOKUP_PATTERN,
	0x100
);
/**
 * @brief lookup_mask sysfs interface
 */
static ACM_SYSFS_BYPASS_ATTR_WO(
	lookup_mask,
	sizeof(struct acmdrv_bypass_lookup),
	ACMDRV_BYPASS_MODULES_COUNT * ACMDRV_BYPASS_NR_RULES,
	ACM_BYPASS_STREAM_IDENT_LOOKUP_MASK,
	0x100
);

/**
 * @brief layer7_pattern sysfs interface
 */
static ACM_SYSFS_BYPASS_ATTR_WO_PADDED(
	layer7_pattern,
	sizeof(((struct acmdrv_bypass_layer7_check *)0)->data),
	ACMDRV_BYPASS_MODULES_COUNT * ACMDRV_BYPASS_NR_RULES,
	ACM_BYPASS_STREAM_IDENT_LAYER7_PATTERN,
	0x100,
	sizeof(struct acmdrv_bypass_layer7_check)
);

/**
 * @brief layer7_mask sysfs interface
 */
static ACM_SYSFS_BYPASS_ATTR_WO_PADDED(
	layer7_mask,
	sizeof(((struct acmdrv_bypass_layer7_check *)0)->data),
	ACMDRV_BYPASS_MODULES_COUNT * ACMDRV_BYPASS_NR_RULES,
	ACM_BYPASS_STREAM_IDENT_LAYER7_MASK,
	0x100,
	sizeof(struct acmdrv_bypass_layer7_check)
);

/**
 * @brief stream_trigger sysfs interface
 */
static ACM_SYSFS_BYPASS_ATTR_WO(
	stream_trigger,
	sizeof(struct acmdrv_bypass_stream_trigger),
	ACMDRV_BYPASS_MODULES_COUNT * (ACMDRV_BYPASS_NR_RULES + 1),
	ACM_BYPASS_DMA_CMD_LOOKUP,
	sizeof(struct acmdrv_bypass_stream_trigger)
);

/**
 * @brief scatter_dma sysfs interface
 */
static ACM_SYSFS_BYPASS_ATTR_WO(
	scatter_dma,
	sizeof(struct acmdrv_bypass_dma_command),
	ACMDRV_BYPASS_MODULES_COUNT * ACMDRV_BYPASS_SCATTER_DMA_CMD_COUNT,
	ACM_BYPASS_SCATTER_DMA,
	sizeof(struct acmdrv_bypass_dma_command)
);

/**
 * @brief prefetch_dma sysfs interface
 */
static ACM_SYSFS_BYPASS_ATTR_WO(
	prefetch_dma,
	sizeof(struct acmdrv_bypass_dma_command),
	ACMDRV_BYPASS_MODULES_COUNT * ACMDRV_BYPASS_PREFETCH_DMA_CMD_COUNT,
	ACM_BYPASS_GATHER_DMA_PREFETCH,
	sizeof(struct acmdrv_bypass_dma_command)
);

/**
 * @brief gather_dma sysfs interface
 */
static ACM_SYSFS_BYPASS_ATTR_WO(
	gather_dma,
	sizeof(struct acmdrv_bypass_dma_command),
	ACMDRV_BYPASS_MODULES_COUNT * ACMDRV_BYPASS_GATHER_DMA_CMD_COUNT,
	ACM_BYPASS_GATHER_DMA_EXEC,
	sizeof(struct acmdrv_bypass_dma_command)
);

/**
 * @brief const_buffer sysfs interface
 */
static ACM_SYSFS_BYPASS_ATTR_RW(
	const_buffer,
	sizeof(struct acmdrv_bypass_const_buffer),
	ACMDRV_BYPASS_MODULES_COUNT,
	ACM_BYPASS_CONSTANT_BUFFER,
	sizeof(struct acmdrv_bypass_const_buffer)
);

/**
 * @brief cntl_ngn_enable sysfs interface
 */
static ACM_SYSFS_BYPASS_REG_ATTR_RW(cntl_ngn_enable,
			      ACM_BYPASS_CONTROL_AREA,
			      ACM_BYPASS_CONTROL_AREA_NGN_ENABLE);

/**
 * @brief cntl_lookup_enable sysfs interface
 */
static ACM_SYSFS_BYPASS_REG_ATTR_RW(cntl_lookup_enable,
			      ACM_BYPASS_CONTROL_AREA,
			      ACM_BYPASS_CONTROL_AREA_LOOKUP_ENABLE);

/**
 * @brief cntl_layer7_enable sysfs interface
 */
static ACM_SYSFS_BYPASS_REG_ATTR_RW(cntl_layer7_enable,
			      ACM_BYPASS_CONTROL_AREA,
			      ACM_BYPASS_CONTROL_AREA_LAYER7_ENABLE);

/**
 * @brief cntl_ingress_policing_enable sysfs interface
 */
static ACM_SYSFS_BYPASS_REG_ATTR_RW(cntl_ingress_policing_enable,
			      ACM_BYPASS_CONTROL_AREA,
			      ACM_BYPASS_CONTROL_AREA_INGRESS_POLICING_ENABLE);

/**
 * @brief cntl_connection_mode sysfs interface
 */
static ACM_SYSFS_BYPASS_REG_ATTR_RW(cntl_connection_mode,
			      ACM_BYPASS_CONTROL_AREA,
			      ACM_BYPASS_CONTROL_AREA_CONNECTION_MODE);

/**
 * @brief cntl_output_disable sysfs interface
 */
static ACM_SYSFS_BYPASS_REG_ATTR_RW(cntl_output_disable,
			      ACM_BYPASS_CONTROL_AREA,
			      ACM_BYPASS_CONTROL_AREA_OUTPUT_DISABLE);

/**
 * @brief cntl_layer7_length sysfs interface
 */
static ACM_SYSFS_BYPASS_REG_ATTR_RW(cntl_layer7_length,
			      ACM_BYPASS_CONTROL_AREA,
			      ACM_BYPASS_CONTROL_AREA_LAYER7_CHECK_LEN);

/**
 * @brief cntl_speed sysfs interface
 */
static ACM_SYSFS_BYPASS_REG_ATTR_RW(cntl_speed,
			      ACM_BYPASS_CONTROL_AREA,
			      ACM_BYPASS_CONTROL_AREA_SPEED);

/**
 * @brief cntl_gather_delay sysfs interface
 */
static ACM_SYSFS_BYPASS_REG_ATTR_RW(cntl_gather_delay,
			      ACM_BYPASS_CONTROL_AREA,
			      ACM_BYPASS_CONTROL_AREA_STATIC_GATHER_DELAY);

/**
 * @brief cntl_ingress_policing_control sysfs interface
 */
static ACM_SYSFS_BYPASS_REG_ATTR_RW(cntl_ingress_policing_control,
			      ACM_BYPASS_CONTROL_AREA,
			      ACM_BYPASS_CONTROL_AREA_INGRESS_POLICING_CTRL);

/**
 * @brief write function for clear_all_fpga
 */
static ssize_t clear_all_fpga_write(struct file *filp, struct kobject *kobj,
				    struct bin_attribute *bin_attr, char *buf,
				    loff_t off, size_t count)
{
	int ret;
	struct acm *acm = kobj_to_acm(kobj);

	if (*((u32 *)buf) == ACMDRV_CLEAR_ALL_PATTERN) {
		enum acmdrv_status status;

		ret = acm_config_remove(acm, &status);
		if (!ret)
			ret = acm_state_set(acm->status, status);
		return ret ? ret : count;
	}

	return -EINVAL;
}

/**
 * @brief Config attribute clear_all_fpga
 */
static BIN_ATTR_WO(clear_all_fpga, sizeof(u32));

/**
 * @brief read function for individual_recovery
 */
static ssize_t individual_recovery_read(struct file *file, struct kobject *kobj,
					struct bin_attribute *bin_attr,
					char *buf, loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(u32);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		u32 timeout;
		unsigned int module = i / ACMDRV_BYPASS_NR_RULES;
		unsigned int rule = i % ACMDRV_BYPASS_NR_RULES;

		ret = redundancy_get_individual_recovery_timeout(
			acm->redundancy, module, rule, &timeout);
		if (ret)
			return ret;

		put_unaligned(timeout, (u32 *)buf);
		buf += elsize;
	}
	return size;
}

/**
 * @brief write function for individual_recovery
 */
static ssize_t individual_recovery_write(struct file *file,
	struct kobject *kobj, struct bin_attribute *bin_attr, char *buf,
	loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(u32);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		unsigned int module = i / ACMDRV_BYPASS_NR_RULES;
		unsigned int rule = i % ACMDRV_BYPASS_NR_RULES;

		u32 timeout = get_unaligned((u32 *)buf);

		ret = redundancy_set_individual_recovery_timeout(
			acm->redundancy, module, rule, timeout);
		if (ret)
			return ret;

		buf += elsize;
	}
	return size;
}

/**
 * @brief Config attribute individual_recovery
 */
static BIN_ATTR_RW(individual_recovery,
	sizeof(struct acmdrv_redun_individual_recovery));

/**
 * @brief read function for base_recovery
 */
static ssize_t base_recovery_read(struct file *file, struct kobject *kobj,
				  struct bin_attribute *bin_attr, char *buf,
				  loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(u32);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		u32 timeout;

		ret = redundancy_get_base_recovery_timeout(acm->redundancy, i,
			&timeout);
		if (ret)
			return ret;

		put_unaligned(timeout, (u32 *)buf);
		buf += elsize;
	}
	return size;
}

/**
 * @brief write function for base_recovery
 */
static ssize_t base_recovery_write(struct file *file, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off, size_t size)
{
	int ret;
	unsigned int i;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t elsize = sizeof(u32);

	ret = sysfs_bin_attr_check(bin_attr, off, size, elsize);
	if (ret)
		return ret;

	foreach_item(i, off, size, elsize) {
		u32 timeout = get_unaligned((u32 *)buf);

		ret = redundancy_set_base_recovery_timeout(acm->redundancy, i,
			timeout);
		if (ret)
			return ret;

		buf += elsize;
	}
	return size;
}

/**
 * @brief Config attribute base_recovery
 */
static BIN_ATTR_RW(base_recovery,
	sizeof(struct acmdrv_redun_base_recovery));


/**
 * @brief sysfs attributes of config section
 */
static struct bin_attribute *config_attributes[] = {
	&bin_attr_configuration_id,

	/* State control */
	&bin_attr_config_state,

	/* Message Buffer configuration */
	&bin_attr_msg_buff_desc,
	&bin_attr_msg_buff_alias,

	&sysfs_bypass_attr_lookup_pattern.bin_attr,
	&sysfs_bypass_attr_lookup_mask.bin_attr,
	&sysfs_bypass_attr_layer7_pattern.bin_attr,
	&sysfs_bypass_attr_layer7_mask.bin_attr,
	&sysfs_bypass_attr_stream_trigger.bin_attr,
	&sysfs_bypass_attr_scatter_dma.bin_attr,
	&sysfs_bypass_attr_prefetch_dma.bin_attr,
	&sysfs_bypass_attr_gather_dma.bin_attr,
	&sysfs_bypass_attr_const_buffer.bin_attr,
	&bin_attr_redund_cnt_tab,
	&bin_attr_redund_status_tab,
	&bin_attr_redund_intseqnum_tab,

	/* Scheduler configuration */
	&bin_attr_sched_down_counter,
	&bin_attr_sched_tab_row,
	&bin_attr_table_status,
	&bin_attr_sched_cycle_time,
	&bin_attr_sched_start_table,
	&bin_attr_emergency_disable,

	/* Bypass Control */
	&sysfs_bypass_reg_attr_cntl_ngn_enable.bin_attr,
	&sysfs_bypass_reg_attr_cntl_lookup_enable.bin_attr,
	&sysfs_bypass_reg_attr_cntl_layer7_enable.bin_attr,
	&sysfs_bypass_reg_attr_cntl_ingress_policing_enable.bin_attr,
	&sysfs_bypass_reg_attr_cntl_connection_mode.bin_attr,
	&sysfs_bypass_reg_attr_cntl_output_disable.bin_attr,
	&sysfs_bypass_reg_attr_cntl_layer7_length.bin_attr,
	&sysfs_bypass_reg_attr_cntl_speed.bin_attr,
	&sysfs_bypass_reg_attr_cntl_gather_delay.bin_attr,
	&sysfs_bypass_reg_attr_cntl_ingress_policing_control.bin_attr,

	&bin_attr_clear_all_fpga,

	&bin_attr_individual_recovery,
	&bin_attr_base_recovery,

	NULL
};

/**
 * @brief Linux attribute group for config section. Configuration read back is
 *          disabled.
 */
static const struct attribute_group config_group = {
	.name = __stringify(ACMDRV_SYSFS_CONFIG_GROUP),
	.bin_attrs = config_attributes,
};

/**
 * @brief initialize sysfs config section
 *
 * @param acm ACM instance
 */
const struct attribute_group *sysfs_config_init(struct acm *acm)
{
	const unsigned int buffers = commreg_read_msgbuf_count(acm->commreg);

	bin_attr_msg_buff_desc.size = buffers * sizeof(msgbuf_desc_t);
	bin_attr_msg_buff_alias.size = buffers *
		sizeof(struct acmdrv_buff_alias);


	if ((acm->if_id >= ACM_IF_4_0) &&
	    !commreg_read_cfg_read_back(acm->commreg))
		/* no change, since read back config is not enabled */
		goto out;

	/* Since interface v.4.0 CFG Read Back flag defines if read back of
	 * configuration is possible.
	 * For pre-4.0 versions the read back is always enabled
	 */
	sysfs_bypass_attr_lookup_pattern.bin_attr.attr.mode = 0644;
	sysfs_bypass_attr_lookup_mask.bin_attr.attr.mode = 0644;
	sysfs_bypass_attr_layer7_pattern.bin_attr.attr.mode = 0644;
	sysfs_bypass_attr_layer7_mask.bin_attr.attr.mode = 0644;
	sysfs_bypass_attr_stream_trigger.bin_attr.attr.mode = 0644;
	sysfs_bypass_attr_scatter_dma.bin_attr.attr.mode = 0644;
	sysfs_bypass_attr_prefetch_dma.bin_attr.attr.mode = 0644;
	sysfs_bypass_attr_gather_dma.bin_attr.attr.mode = 0644;

out:
	return &config_group;
}

/**
 * exit sysfs config section
 *
 * @param acm ACM instance
 */
void sysfs_config_exit(struct acm *acm)
{
	/* nothing to do */
}
/** @} acmsysfsconfigimpl */

