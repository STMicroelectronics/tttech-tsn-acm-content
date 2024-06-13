/**
 * @file configuration.h
 *
 * Configuration file access
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <stddef.h>
#include <stdbool.h>
#include <sys/queue.h>

struct messagebuffer;
struct worker;

/**
 * @brief Configuration from configuration file
 */
struct configuration_entry {
	int index;		/**< config index */
	char *buffer_name;	/**< name of message buffer */
	int (*function)(struct worker *);	/**< worker function */
	int (*prepare_function)(struct worker *);/**< worker prepare function */
	bool use_timestamp;
	unsigned int size;	/**< data size in bytes */
	unsigned int time_offs;	/**< data xfer time offset in us */
	unsigned int period;	/**< buffer cycle period */

	STAILQ_ENTRY(configuration_entry) entries; /**< a list entry */

	struct messagebuffer *msgbuf;
	struct worker *worker;
};

STAILQ_HEAD(configuration_list, configuration_entry);

int parse_configuration(const char *filename,
			struct configuration_list *configurations);
void empty_configurations(struct configuration_list *configurations);
void log_configurations(int loglevel,
			struct configuration_list *configurations);

static inline void configuration_set_worker(struct configuration_entry *config,
	struct worker *worker)
{
	if (!config)
		return;

	config->worker = worker;
}

#endif /* CONFIGURATION_H_ */
