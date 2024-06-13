/**
 * @file monitor.c
 *
 * Data Monitoring
 *
 * These functions are linked in dynamically depending on the configuration.
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

#include "monitor.h"
#include "worker.h"
#include "logging.h"
#include "messagebuffer.h"
#include "dump.h"

/*
 * data format
 * 0 .. messagebuffer index
 * 1 .. 0
 * 2 - 3 ... message size (little endian)
 * 4 - n .. received message
 */

void monitor_send_rxbuffer(struct worker *worker, int rx_msg_len)
{
	int ret;
	uint8_t *buffer = worker->monitor.udp_buffer;
	uint16_t size = worker->transfer.size;

	buffer[0] = worker->config->msgbuf->alias.idx;
	buffer[1] = 0;
	*((uint16_t *)(&buffer[2])) = htole16(size);
	if (rx_msg_len > 0)
		memcpy(&buffer[4], worker->transfer.data, rx_msg_len);
	else
		memcpy(&buffer[4], worker->transfer.data, size);

	ret = sendto(worker->monitor.udp_socket, buffer, size + 4, 0,
		(struct sockaddr *)&worker->monitor.udp_addr,
		sizeof(worker->monitor.udp_addr));

	if (ret < 0) {
		char ipaddr[INET_ADDRSTRLEN];

		inet_ntop(AF_INET, &(worker->monitor.udp_addr.sin_addr),
			  ipaddr, INET_ADDRSTRLEN);
		LOGGING_ERR("%s: Cannot access %s:%d: %s", __func__, ipaddr,
			worker->monitor.udp_addr.sin_port, strerror(errno));
	}
}

/**
 * @brief dump diagnostic data
 */
int monitor_dump_diag(struct client *client, const char *cmd)
{
	int ret = 0;
	int offs, mult, cnt;
	struct diag_worker *diag_worker;
	char *command = strdup(cmd);
	char *p = command;

	/* parse command parameters */
	strtok(p, " ");
	p = strtok(NULL, " ");
	offs = p ? atoi(p) : 0;
	if (offs < 0) {
		ret = -EINVAL;
		goto out_free_cmd;
	}
	p = strtok(NULL, " ");
	mult = p ? atoi(p) : 1;
	if (mult <= 0) {
		ret = -EINVAL;
		goto out_free_cmd;
	}
	p = strtok(NULL, " ");
	cnt = p ? atoi(p) : 1;
	if (cnt < 1) {
		ret = -EINVAL;
		goto out_free_cmd;
	}

	diag_worker = create_diag_worker(client, offs, mult, cnt);
	if (!diag_worker) {
		ret = -ENOMEM;
		goto out_free_cmd;
	}
	ret = start_diag_worker(diag_worker);
	if (ret)
		goto out;

	ret = wait_diag_worker_finished(diag_worker);
	if (ret)
		goto out;

	ret = monitor_dump_diag_data(client, diag_worker);

out:
	destroy_diag_worker(diag_worker);

out_free_cmd:
	free(command);
	return ret;
}

