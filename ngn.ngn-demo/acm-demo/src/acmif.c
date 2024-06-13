/**
 * @file acmif.c
 *
 * Helpers to access ACM driver's SYSF interface
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "acmif.h"
#include "logging.h"

/**
 * @brief Read binary data from SYSFS
 *
 * @param what Filename path relative to #ACMDEV_BASE
 * @param dest Pointer to destination
 * @param offs offset of data within sysfs attribute
 * @param size Amount of data to be read
 * @return number of read bytes if positive, negative indicate error
 */
int acmif_sysfs_read(const char *what, void *dest, off_t offs, size_t size)
{
	int fd;
	int ret;
	char filename[512];

	ret = snprintf(filename, sizeof(filename), "%s%s", ACMDEV_BASE, what);
	if (ret >= sizeof(filename)) {
		LOGGING_ERR("%s: filename too long", what);
		return -EINVAL;
	}

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		LOGGING_ERR("%s: %s", what, strerror(errno));
		return -errno;
	}

	ret = lseek(fd, offs, SEEK_SET );
	if (ret < 0)
		goto close;
	ret = read(fd, dest, size);

close:
	close(fd);

	return ret;
}

/**
 * @brief Read uint32_t ASCII data from SYSFS
 *
 * @param what Filename path relative to #ACMDEV_BASE
 * @param dest Pointer to destination
 */
int acmif_sysfs_read_uint32(const char *what, uint32_t *dest)
{
	int fd;
	int ret;
	char filename[512];
	char buffer[32] = { 0 };
	unsigned long value;

	ret = snprintf(filename, sizeof(filename), "%s%s", ACMDEV_BASE, what);
	if (ret >= sizeof(filename) || ret < 0) {
		LOGGING_ERR("%s: filename too long", what);
		return ret < 0 ? ret : -EINVAL;
	}

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		LOGGING_ERR("%s: %s", what, strerror(errno));
		return -errno;
	}

	ret = read(fd, buffer, sizeof(buffer));
	if (ret < 0) {
		LOGGING_ERR("%s: %s", what, strerror(errno));
		ret = -errno;
		goto close;
	}

	value = strtoul(buffer, NULL, 0);
	if (errno) {
		LOGGING_ERR("%s: %s", what, strerror(errno));
		ret = -errno;
		goto close;
	}

	*dest = (uint32_t)value;
	ret = 0;

close:
	close(fd);

	return ret;
}
