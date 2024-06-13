/**
 * @file dump.h
 *
 * Formatting monitored data for dumping
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#ifndef DUMP_H_
#define DUMP_H_

#include <stdint.h>

struct client;
struct diag_worker;

int monitor_dump(struct client *client);
int monitor_dump2(struct client *client);
int monitor_dump_cycles(struct client *client);
int monitor_dump_diag_data(struct client *client,
	struct diag_worker *diag_worker);


#endif /* DUMP_H_ */
