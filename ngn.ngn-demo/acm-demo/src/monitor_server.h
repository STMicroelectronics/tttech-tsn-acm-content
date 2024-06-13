/**
 * @file monitor_server.h
 *
 * Server for monitor data
 *
 * These functions are linked in dynamically depending on the configuration.
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#ifndef MONITOR_SERVER_H_
#define MONITOR_SERVER_H_

int monitor_server_start(void);
void monitor_server_stop(void);

struct client;
int client_send(struct client *client, const char *fmt, ...)
	__attribute__ ((__format__ (__printf__, 2, 3)));

#endif /* MONITOR_SERVER_H_ */
