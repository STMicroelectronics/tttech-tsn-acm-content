/**
 * @file acmif.h
 *
 * Helpers to access ACM driver's SYSF interface
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#ifndef ACMIF_H_
#define ACMIF_H_

#include <stdio.h>
#include <linux/acm/acmdrv.h>
#include "logging.h"

#ifndef ACMDEV_BASE
/**
 * @brief default for base SYSFS directory for ACM driver access
 *
 * This might be overridden setting it as compiler flag for testing purposes on
 * non-target systems.
 */
#define ACMDEV_BASE "/sys/devices/acm/"
#endif

/**
 * @brief SYFS name for message buffer alias access
 */
#define ACMDEV_MSG_BUFF_ALIAS_NAME "msg_buff_alias"

/**
 * @brief SYFS name for message buffer descriptor access
 */
#define ACMDEV_MSG_BUFF_DESC_NAME "msg_buff_desc"

/**
 * @brief SYFS name for message buffer datawidth access
 */
#define ACMDEV_MSG_BUFF_DATAWIDTH "msgbuf_datawidth"

int acmif_sysfs_read(const char *what, void *dest, off_t offs, size_t size);
int acmif_sysfs_read_uint32(const char *what, uint32_t *dest);

/**
 * @brief read consecutive messagebuffer alias entries
 */
static inline int acmif_get_buff_alias_array(int from, int to,
					     struct acmdrv_buff_alias *buffalias)
{
	return acmif_sysfs_read(stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/"
				ACMDEV_MSG_BUFF_ALIAS_NAME,
				buffalias,
				from * sizeof(*buffalias),
				(to - from + 1) * sizeof(*buffalias));
}

/**
 * @brief read single messagebuffer alias entry
 */
static inline int acmif_get_buff_alias(int index,
				       struct acmdrv_buff_alias *buffalias)
{
	return acmif_get_buff_alias_array(index, index, buffalias);
}

/**
 * @brief read consecutive messagebuffer descriptor entries
 */
static inline int acmif_get_buff_desc_array(int from, int to,
					    struct acmdrv_buff_desc *buffdesc)
{
	return acmif_sysfs_read(stringify(ACMDRV_SYSFS_CONFIG_GROUP) "/"
				ACMDEV_MSG_BUFF_DESC_NAME,
				buffdesc,
				from * sizeof(*buffdesc),
				(to - from + 1) * sizeof(*buffdesc));
}

/**
 * @brief read single messagebuffer descriptor entry
 */
static inline int acmif_get_buff_desc(int index, struct acmdrv_buff_desc *buffdesc)
{
	return acmif_get_buff_desc_array(index, index, buffdesc);
}

/**
 * @brief read and calculate message buffer size from messagebuffer descriptor
 */
static inline unsigned int acmif_msgbuf_size(struct acmdrv_buff_desc *buffdesc)
{
	int ret;
	unsigned int datawidth;

	ret = acmif_sysfs_read_uint32(stringify(ACMDRV_SYSFS_STATUS_GROUP) "/"
		ACMDEV_MSG_BUFF_DATAWIDTH, &datawidth);
	if (ret)
		return 0;

	return acmdrv_buff_desc_sub_buffer_size_read(buffdesc) * datawidth;
}


#endif /* ACMIF_H_ */
