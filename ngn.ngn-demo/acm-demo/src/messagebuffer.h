/**
 * @file messagebuffer.h
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
#ifndef MESSAGEBUFFER_H_
#define MESSAGEBUFFER_H_

#include <stddef.h>
#include <sys/queue.h>
#include <linux/acm/acmdrv.h>

STAILQ_HEAD(messagebuffer_list, messagebuffer);

/**
 * @brief Provide entire message buffer data
 */
struct messagebuffer {
	struct acmdrv_buff_alias alias;
	struct acmdrv_buff_desc desc;

	STAILQ_ENTRY(messagebuffer) entries;
};

int read_messagebuffers(void);
void empty_messagebuffers(void);
struct messagebuffer *get_messagebuffer_by_name(const char *name);
void log_messagebuffers(int loglvl);

#endif /* MESSAGEBUFFER_H_ */
