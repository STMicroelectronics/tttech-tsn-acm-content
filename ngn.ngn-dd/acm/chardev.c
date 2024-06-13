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
 * @file chardev.c
 * @brief ACM Driver Message Buffer Character Device
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup communication ACM Communication
 * @brief ACM character device implementation
 *
 * @{
 */
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mutex.h>

#include "acm-module.h"
#include "chardev.h"
#include "msgbuf.h"
#include "state.h"
#include "commreg.h"

/**
 * @brief ACM character device instance
 */
struct acm_dev {
	struct acm *acm;		/**< associated ACM instance */
	int idx;			/**< device index */
	struct device *dev;		/**< ACM device uses a Linux device */
	struct cdev cdev;		/**< ACM device is a Linux cdev */
	struct mutex cdev_mutex;	/**< lock (de)activation */

	u64 id;				/**< stream id */
	size_t size;			/**< message buffer size in bytes */

	bool active;			/**< state of ACM device */
	bool timestamp;			/**< time stamp configuration */
};

/**
 * @brief read method for ACM message buffer devices
 * @param file the file to read from
 * @param buf the buffer to read to
 * @param size the maximum number of bytes to read
 * @param ppos the current position in the file
 * @return number of bytes read or errno
 */
static ssize_t acm_dev_read(struct file *file, char __user *buf, size_t size,
			    loff_t *ppos)
{
	ssize_t ret;
	struct acm_dev *adev = file->private_data;
	struct acm *acm = adev->acm;

	ret = mutex_lock_interruptible(&adev->cdev_mutex);
	if (ret)
		return ret;

	if (!acm_state_is_running(acm->status)) {
		dev_dbg(adev->dev, "not running\n");
		ret = -EIO;
		goto unlock;
	}

	if (*ppos != 0) {
		dev_dbg(adev->dev, "reading from offset %llu not allowed\n",
			*ppos);
		ret = -EINVAL;
		goto unlock;
	}

	if (size  > adev->size) {
		dev_dbg(adev->dev, "truncating message to %zu\n", adev->size);
		size = adev->size;
	}

	ret = msgbuf_read_to_user(acm->msgbuf, adev->idx, buf, size);
	if (ret)
		goto unlock;

	ret = size;

unlock:
	mutex_unlock(&adev->cdev_mutex);
	return ret;
}

/**
 * @brief write method for ACM message buffer devices
 */
static ssize_t acm_dev_write(struct file *file, const char __user *buf,
			     size_t size, loff_t *ppos)
{
	ssize_t ret;
	struct acm_dev *adev = file->private_data;
	struct acm *acm = adev->acm;

	ret = mutex_lock_interruptible(&adev->cdev_mutex);
	if (ret)
		return ret;

	if (!acm_state_is_running(acm->status)) {
		dev_dbg(adev->dev, "not running\n");
		ret = -EIO;
		goto unlock;
	}

	if (*ppos != 0) {
		dev_dbg(adev->dev, "reading from offset %llu not allowed\n",
			*ppos);
		ret = -EINVAL;
		goto unlock;
	}

	if (size  > adev->size) {
		dev_dbg(adev->dev, "truncating message to %zu\n", adev->size);
		size = adev->size;
	}

	if (msgbuf_write_from_user(acm->msgbuf, adev->idx, buf, size)) {
		ret = -EFAULT;
		goto unlock;
	}

	ret = size;
unlock:
	mutex_unlock(&adev->cdev_mutex);

	return ret;
}

/**
 * @brief open method for ACM message buffer devices
 */
static int acm_dev_open(struct inode *inode, struct file *file)
{
	struct acm_dev *adev;

	adev = container_of(inode->i_cdev, struct acm_dev, cdev);
	file->private_data = adev;

	return 0;
}

/**
 * @brief release method for ACM message buffer devices
 */
static int acm_dev_cdev_release(struct inode *inode, struct file *file)
{
	return 0;
}

/**
 * @brief Linux file_operations for ACM as cdev
 */
static const struct file_operations acm_dev_fops = {
	.owner = THIS_MODULE,
	.read = acm_dev_read,
	.write = acm_dev_write,
	.open = acm_dev_open,
	.release = acm_dev_cdev_release,
};

/**
 * activate an ACM device
 */
int __must_check acm_dev_activate(struct acm_dev *adev, const char *alias,
				  u64 id)
{
	int ret;
	u32 desc_type;
	dev_t base = adev->acm->devt;
	struct device *parent = &adev->acm->dev;

	dev_t devt = MKDEV(MAJOR(base), MINOR(base) + adev->idx);

	ret = mutex_lock_interruptible(&adev->cdev_mutex);
	if (ret)
		return ret;

	if (adev->active) {
		dev_err(parent,
			"%s: ACM device%d already active", __func__, adev->idx);
		ret = -EBUSY;
		goto unlock;
	}

	/* do not activate devices with invalid message buffer descriptor */
	if (!msgbuf_is_valid(adev->acm->msgbuf, adev->idx)) {
		dev_err(parent,
			"%s: Messagebuffer %d not valid", __func__, adev->idx);
		ret = -EIO;
		goto unlock;
	}

	adev->id = id;

	/* message buffer size in bytes */
	adev->size = msgbuf_size(adev->acm->msgbuf, adev->idx);
	desc_type = msgbuf_type(adev->acm->msgbuf, adev->idx);

	cdev_init(&adev->cdev, &acm_dev_fops);
	adev->cdev.owner = THIS_MODULE;

	ret = cdev_add(&adev->cdev, devt, 1);
	if (ret) {
		dev_err(parent,
			"%s: cdev_add() for ACM device%d (%s) failed: %d",
			__func__, adev->idx, alias, ret);
		goto unlock;
	}

	adev->dev = device_create(acm_class, parent, devt, NULL, alias);
	if (IS_ERR(adev->dev)) {
		dev_err(parent,
			"%s: device_create() for ACM device%d (%s) failed: %d",
			__func__, adev->idx, alias, ret);
		cdev_del(&adev->cdev);
		goto unlock;
	}

	adev->active = true;
	dev_dbg(parent, "ACM device%d (%s) activated successfully",
		adev->idx, alias);

unlock:
	mutex_unlock(&adev->cdev_mutex);
	return ret;
}

/**
 * @brief deactivate an ACM device
 */
void acm_dev_deactivate(struct acm_dev *adev)
{
	struct device *parent = &adev->acm->dev;

	mutex_lock(&adev->cdev_mutex);
	if (!acm_dev_is_active(adev))
		goto out;

	dev_dbg(parent, "ACM device%d (%s) deactivated", adev->idx,
		acm_dev_get_name(adev));

	adev->active = false;
	device_destroy(acm_class, adev->cdev.dev);
	cdev_del(&adev->cdev);
out:
	mutex_unlock(&adev->cdev_mutex);
}

/**
 * @brief check if ACM device uses time stamping
 */
bool acm_dev_get_timestamping(const struct acm_dev *adev)
{
	return adev->timestamp;
}

/**
 * @brief configure ACM device to use time stamping
 */
void acm_dev_set_timestamping(struct acm_dev *adev, bool enable)
{
	adev->timestamp = enable;
}

/**
 * @brief get name of ACM device
 */
const char *acm_dev_get_name(const struct acm_dev *adev)
{
	if (adev->active)
		return dev_name(adev->dev);

	return NULL;
}

/**
 * @brief check if ACM device is active
 */
bool acm_dev_is_active(const struct acm_dev *adev)
{
	return adev->active;
}

/**
 * @brief get associated stream-id of  ACM devicee
 */
u64 acm_dev_get_id(const struct acm_dev *adev)
{
	return adev->id;
}

/**
 * @brief helper to get ACM device pointer per index
 */
struct acm_dev *acm_dev_get_device(struct acm_dev *first, int i)
{
	return &first[i];
}

/**
 * @brief create ACM device list
 */
struct acm_dev *acm_dev_create(struct acm *acm, size_t nr)
{
	int i;
	struct acm_dev *aadev;
	struct platform_device *pdev = acm->pdev;
	struct device *parent = &pdev->dev;

	aadev = devm_kcalloc(parent, nr, sizeof(*aadev), GFP_KERNEL);
	if (!aadev)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < nr; ++i) {
		struct acm_dev *adev = &aadev[i];

		adev->acm = acm;
		adev->idx = i;
		adev->active = false;

		mutex_init(&adev->cdev_mutex);
	}

	return aadev;
}

/**
 * @brief destroy ACM device list
 */
void acm_dev_destroy(struct acm *acm)
{
	int i;

	for (i = 0; i < commreg_read_msgbuf_count(acm->commreg); ++i)
		acm_dev_deactivate(&acm->devices[i]);
}
/**@} communication*/

