/**
 * @file monitor_server.c
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
#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdarg.h>

#include "monitor.h"
#include "logging.h"
#include "dump.h"

#define MAX_CONN		10

struct monitor_server {
	unsigned short port;
	int fd;
	struct sockaddr_in addr;
	bool is_running;
	pthread_t tid;
};

struct client {
	int fd;
	char outbuf[0x4000];
	bool close_request;
};

int client_send(struct client *client, const char *fmt, ...)
{
	int ret;
	int fd = client ? client->fd : STDERR_FILENO;
	va_list args;

	va_start(args, fmt);

	ret = vdprintf(fd, fmt, args);
	va_end(args);

	return ret < 0 ? ret : 0;
}


static struct monitor_server the_server;

static int process(struct client *client, const char *cmd)
{
	int ret;

	LOGGING_DEBUG("monitor_server: processing %s", cmd);

	if (!strcmp(cmd, "DUMP")) {
		ret = monitor_dump(client);
	} else if (!strcmp(cmd, "DUMP2")) {
		ret = monitor_dump2(client);
	} else if (!strcmp(cmd, "DUMPCYC")) {
		/* dump monitor data */
		ret = monitor_dump_cycles(client);
	} else if (!strncmp(cmd, "DIAGNOSTICS", strlen("DIAGNOSTICS"))) {
		/* get diagnostics parameter:
		 * DIAGNOSTICS <cycle offset in us> <cycle interval multiplicator> <count>
		 */
		ret = monitor_dump_diag(client, cmd);
		if (ret < 0)
			LOGGING_ERR("Monitor server: monitor_dump_diag(%s) failed: %s",
				__func__, cmd, strerror(ret));
		client->close_request = true;
	} else {
		LOGGING_ERR("Monitor server: Unknown command %s", cmd);
		snprintf(client->outbuf, sizeof(client->outbuf),
			"%s: Unknown command\n", cmd);
		send(client->fd, client->outbuf, sizeof(client->outbuf), 0);
		ret = -EINVAL;
	}

	if (ret > 0)
		ret = 0;

	return ret;
}

static void *client_handler(void *data)
{
	int ret;
	struct client client;

	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	char clientaddr[16];

	char rcvdata[64];
	char cmd[sizeof(rcvdata) + 1];
	const struct timeval rcvtimeout = {
		.tv_sec = 5,
		.tv_usec = 0
	};

	client.fd = (intptr_t)data;
	client.close_request = false;

	getpeername(client.fd, (struct sockaddr *)&addr, &addr_size);
	inet_ntop(AF_INET, &(addr.sin_addr), clientaddr, sizeof(clientaddr));
	LOGGING_INFO("%s: Connected", clientaddr);

	pthread_setname_np(pthread_self(), clientaddr);

	ret = setsockopt(client.fd, SOL_SOCKET, SO_RCVTIMEO, &rcvtimeout,
		sizeof(rcvtimeout));
	if (ret < 0){
		LOGGING_ERR("%s: SO_RCVTIMEO on client socket failed: %s",
			    clientaddr, strerror(errno));
		goto out;
	}

	while (the_server.is_running && !client.close_request) {
		int len;

		memset(rcvdata, 0, sizeof(rcvdata));

		ret = recv(client.fd, rcvdata, sizeof(rcvdata), 0);
		if (ret == 0) { /* client disconnected */
			LOGGING_DEBUG("%s: Disconnected", clientaddr);
			break;
		}

		if (ret < 0) {
			switch (errno) {
			case EAGAIN: /* timeout: close connection */
				LOGGING_DEBUG("%s: Connection timeout",
					      clientaddr);
				break;
			default:
				LOGGING_DEBUG("%s: Receive failure: %d (%s)",
					      clientaddr, errno,
					      strerror(errno));
				break;
			}
			break;
		}

		snprintf(cmd, sizeof(cmd), "%s", rcvdata);
		len = strlen(cmd);
		if (cmd[len - 1] == '\n')
			cmd[len - 1] = 0x00;
		if (len > 1) {
			ret = process(&client, cmd);
			LOGGING_DEBUG("%s: Processed %s: %s",
				      clientaddr, cmd, strerror(ret));
			if (client.close_request)
				LOGGING_DEBUG("%s: Close requested",
					clientaddr);
			if (ret < 0)
				break;
		}
	}

out:
	close(client.fd);
	LOGGING_INFO("%s: Connection close", clientaddr);

	return NULL;
}

void *monitor_server_run(void *unused)
{
	int ret;
	int optval;

	LOGGING_DEBUG("monitor-server: Running server thread");

	pthread_setname_np(pthread_self(), "monitor-server");

	the_server.port = get_param_portno();
	the_server.fd = socket(AF_INET, SOCK_STREAM, 0);
	if (the_server.fd < 0) {
		LOGGING_ERR("monitor-server: Cannot open monitoring socket: %s",
			    strerror(errno));
		ret = -errno;
		goto out;
	}
	LOGGING_DEBUG("monitor-server: Opened monitor server on socket %d successfully",
		      the_server.fd);

	optval = 1;
	ret = setsockopt(the_server.fd, SOL_SOCKET, SO_REUSEADDR,
		(const void *)&optval, sizeof(optval));
	if (ret < 0){
		LOGGING_ERR("monitor-server: SO_REUSEADDR on monitor socket failed: %s",
			    strerror(errno));
		ret = -errno;
		goto out_close;
	}
	LOGGING_DEBUG("monitor-server: Successfully set SO_REUSEADDR on monitor socket");

	the_server.addr.sin_family = AF_INET;
	the_server.addr.sin_addr.s_addr = INADDR_ANY;
	the_server.addr.sin_port = htons(the_server.port);
	the_server.is_running = true;

	if (bind(the_server.fd, (struct sockaddr *)&the_server.addr,
		sizeof(the_server.addr)) < 0) {
		LOGGING_ERR("monitor-server: Cannot bind monitoring socket: %s",
			    strerror(errno));
		ret = -errno;
		goto out_close;
	}

	ret = listen(the_server.fd, MAX_CONN);
	if (ret < 0) {
		LOGGING_ERR("monitor-server: Cannot listen at monitoring socket: %s",
			    strerror(errno));
		ret = -errno;
		goto out_close;
	}
	LOGGING_INFO("monitor-server: Listening to incoming monitor connection at port %d",
		     the_server.port);

	while (the_server.is_running) {
		int fd;
		struct sockaddr_in addr;
		socklen_t addrlen;
		pthread_t thread;
		pthread_attr_t attr;

		addrlen = sizeof(addr);

		fd = accept(the_server.fd,
			(struct sockaddr *)&addr, &addrlen);
		if (fd < 0) {
			switch (errno) {
			case EBADF: /* terminated */
				ret = 0;
				break;
			default:
				LOGGING_ERR("monitor-server: accept() failed: %s",
					strerror(errno));
				ret = -errno;
				break;
			}
			break;
		}

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		ret = pthread_create(&thread, &attr, client_handler,
			(void *)(intptr_t)fd);
		if (ret)
			break;

	}

	LOGGING_DEBUG("monitor-server: thread stopped");

out_close:
	if (the_server.fd > 0)
		close(the_server.fd);
out:
	return (void *)(intptr_t)ret;
}

int monitor_server_start(void)
{
	LOGGING_DEBUG("monitor-server: Starting");
	memset(&the_server, 0, sizeof(the_server));

	return pthread_create(&the_server.tid, NULL, monitor_server_run, NULL);
}

void monitor_server_stop(void)
{
	int fd = the_server.fd;

	if (fd > 0) {
		LOGGING_DEBUG("monitor-server: Stopping");

		the_server.is_running = false;
		the_server.fd = -1;
		close(fd);
		pthread_join(the_server.tid, NULL);
	}
}
