/**
 * @file messagebuffer.c
 *
 * Message buffer representation
 *
 * These functions are linked in dynamically depending on the configuration.
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "messagebuffer.h"
#include "logging.h"
#include "acmif.h"

#define NUM_MESSAGEBUFFERS 32

static struct messagebuffer_list messagebuffers =
	STAILQ_HEAD_INITIALIZER(messagebuffers);

/**
 * Read info of all valid message buffers from IP
 */
int read_messagebuffers(void)
{
	int ret;
	int i;
	struct acmdrv_buff_alias buffalias;
	struct acmdrv_buff_desc buffdesc;
	struct messagebuffer *msgbuf;

	for (i = 0; i < NUM_MESSAGEBUFFERS; ++i) {
		ret = acmif_get_buff_desc(i, &buffdesc);
		if (ret < 0)
			goto out;

		/* skip invalid message buffers */
		if (!acmdrv_buff_desc_valid_read(&buffdesc))
			continue;

		ret = acmif_get_buff_alias(i, &buffalias);
		if (ret < 0)
			goto out;

		msgbuf = malloc(sizeof(*msgbuf));
		if (!msgbuf) {
			ret = -ENOMEM;
			goto out;
		}
		msgbuf->alias = buffalias;
		msgbuf->desc = buffdesc;

		LOGGING_DEBUG("%s: inserting message buffer %d", __func__, i);
		LOGGING_DEBUG("%s: descriptor = 0x%08x", __func__, i, msgbuf->desc.desc);
		LOGGING_DEBUG("%s: alias[%d].idx = %u", __func__, i, msgbuf->alias.idx);
		LOGGING_DEBUG("%s: alias[%d].id = %llu", __func__, i, msgbuf->alias.id);
		LOGGING_DEBUG("%s: alias[%d].alias = %s", __func__, i, msgbuf->alias.alias);

		STAILQ_INSERT_TAIL(&messagebuffers, msgbuf, entries);
	}
	ret = 0;

out:
	return ret;
}

void empty_messagebuffers(void)
{
	struct messagebuffer *msgbuf;

	STAILQ_FOREACH(msgbuf, &messagebuffers, entries) {
		STAILQ_REMOVE(&messagebuffers, msgbuf, messagebuffer, entries);
		free(msgbuf);
	}
}

struct messagebuffer *get_messagebuffer_by_name(const char *name)
{
	struct messagebuffer *msgbuf;

	STAILQ_FOREACH(msgbuf, &messagebuffers, entries) {
		if (!strcmp(msgbuf->alias.alias, name))
			return msgbuf;
	}

	return NULL;
}

void log_messagebuffers(int loglvl)
{
	struct messagebuffer *msgbuf;

	STAILQ_FOREACH(msgbuf, &messagebuffers, entries) {
		LOG(loglvl,
		    "message buffer: name = %s, idx = %d, offset = %u, type = %u, size = %u",
		    msgbuf->alias.alias, msgbuf->alias.idx,
		    acmdrv_buff_desc_offset_read(&msgbuf->desc),
		    acmdrv_buff_desc_type_read(&msgbuf->desc),
		    acmif_msgbuf_size(&msgbuf->desc));
	}
}

