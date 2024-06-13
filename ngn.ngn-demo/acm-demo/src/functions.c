/**
 * @file functions.c
 *
 * Available worker functions
 *
 * These functions are linked in dynamically depending on the configuration.
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include "worker.h"
#include "monitor.h"
#include "sps_demo.h"

static inline int do_write(struct worker *worker)
{
	int ret;

	struct transfer *xfer = &worker->transfer;

	monitor_function_start(worker);
	ret = write(xfer->msgbuf, xfer->data, xfer->size);
	monitor_function_end(worker);
	if (ret != xfer->size)
		return -EIO;

	monitor_packet_transferred(worker);
	return ret;
}

/*
 * tx counter format: |lo|xx|yy|hi|
 * pp: (counter & 0x3f) + ' '
 *
 * data block (total size as configured in message buffer):
 * |1B|lo|xx|02|00|lo|xx|yy|hi|pp|pp|...|pp|
 */
/* cppcheck-suppress unusedFunction */
int write_stream_1(struct worker *worker)
{
	struct transfer *xfer = &worker->transfer;
	uint8_t data = TO_ASCII(xfer->packet_id);

	xfer->data[0] = 0x1B;
	TO_UINT16(xfer->data[1]) = htole16(xfer->packet_id);
	TO_UINT16(xfer->data[3]) = htole16(2);
	TO_UINT32(xfer->data[5]) = htole32(xfer->packet_id);

	memset(&xfer->data[9], data, xfer->size - 9);
	return do_write(worker);
}

/*
 * tx counter format: |lo|xx|yy|hi|
 * pp: (counter & 0x3f) + ' '
 *
 * data block (total size as configured in message buffer):
 * |lo|xx|1B|lo|xx|02|00|lo|xx|yy|hi|pp|pp|...|pp|
 */
/* cppcheck-suppress unusedFunction */
int write_stream_seq_1(struct worker *worker)
{
	struct transfer *xfer = &worker->transfer;
	uint8_t data = TO_ASCII(xfer->packet_id);

	TO_UINT16(xfer->data[0]) = htole16(xfer->packet_id);
	xfer->data[2] = 0x1B;
	TO_UINT16(xfer->data[3]) = htole16(xfer->packet_id);
	TO_UINT16(xfer->data[5]) = htole16(2);
	TO_UINT32(xfer->data[7]) = htole32(xfer->packet_id);

	memset(&xfer->data[11], data, xfer->size - 11);
	return do_write(worker);
}

/* cppcheck-suppress unusedFunction */
int prepare_write_stream_x_y(struct worker *worker)
{
	struct transfer *xfer = &worker->transfer;

	if (worker->config->size < 10) {
		LOGGING_ERR("%s: Configured data size < 10", worker->name);
		return -EINVAL;
	}

	if (xfer->size / worker->config->size == 0) {
		LOGGING_ERR("%s: Configured data size(%u) > Message buffer size(%u)",
			worker->name, worker->config->size, xfer->size);
		return -EINVAL;
	}

	return 0;
}

/*
 * tx counter format: |lo|xx|yy|hi|
 * pp: (counter & 0x3f) + ' '
 *
 * data block (total size as configured in message buffer):
 * |1b]lo|xx|02|00|lo|xx|yy|hi|pp|..|pp| <-- repeat for entire message buffer
 * \--- size of configuration entry ---/     size
 */
/* cppcheck-suppress unusedFunction */
int write_stream_x_y(struct worker *worker)
{
	uint8_t *p;
	struct transfer *xfer = &worker->transfer;
	uint8_t data = TO_ASCII(xfer->packet_id);

	for (p = xfer->data;
	     p <= &xfer->data[xfer->size - worker->config->size];
	     p += worker->config->size) {
		p[0] = 0x1B;
		TO_UINT16(p[1]) = htole16(xfer->packet_id);
		TO_UINT16(p[3]) = htole16(2);
		TO_UINT32(p[5]) = htole32(xfer->packet_id);

		memset(&p[9], data, worker->config->size - 9);
	}

	return do_write(worker);
}

/* cppcheck-suppress unusedFunction */
int prepare_write_stream_seq_x_y(struct worker *worker)
{
	struct transfer *xfer = &worker->transfer;

	if (worker->config->size < 10) {
		LOGGING_ERR("%s: Configured data size < 10", worker->name);
		return -EINVAL;
	}

	if ((xfer->size - 2)/ worker->config->size == 0) {
		LOGGING_ERR("%s: Configured data size(%u) > Message buffer size(%u)",
			worker->name, worker->config->size,
			xfer->size - 2);
		return -EINVAL;
	}

	return 0;
}

/*
 * tx counter format: |lo|xx|yy|hi|
 * pp: (counter & 0x3f) + ' '
 *
 * data block (total size as configured in message buffer):
 * |lo|xx|1B|lo|xx|02|00|lo|xx|yy|hi|pp|..|pp|
 * |lo|xx|1B|lo|xx|02|00|lo|xx|yy|hi|pp|..|pp|
 *       \--- size of configuration entry ---/ <-- repeated!
 */
/* cppcheck-suppress unusedFunction */
int write_stream_seq_x_y(struct worker *worker)
{
	uint8_t *p;
	struct transfer *xfer = &worker->transfer;
	uint8_t data = TO_ASCII(xfer->packet_id);

	TO_UINT16(xfer->data[0]) = htole16(xfer->packet_id);
	for (p = &xfer->data[2];
	     p <= &xfer->data[xfer->size - worker->config->size] + 2;
	     p += worker->config->size) {
		p[0] = 0x1B;
		TO_UINT16(p[1]) = htole16(xfer->packet_id);
		TO_UINT16(p[3]) = htole16(2);
		TO_UINT32(p[5]) = htole32(xfer->packet_id);

		memset(&p[9], data, worker->config->size - 9);
	}

	return do_write(worker);
}

static int read_stream(struct worker *worker)
{
	int len, ret;
	struct transfer *xfer = &worker->transfer;

	monitor_function_start(worker);
	len = read(xfer->msgbuf, xfer->data, xfer->size);
	if (len < 0) {
		/* do not log ENODATA */
		if (errno != ENODATA)
			LOGGING_ERR("%s: %s() failed: %s", worker->name,
				__func__, strerror(errno));
		ret = -errno;
		goto out;
	}
	monitor_function_end(worker);

	xfer->rx_packet_id = le32toh(TO_UINT32(xfer->data[5]));

	ret = monitor_rx_timestamp(worker);
	if (ret)
		goto out;
	ret = monitor_packet_identifier(worker, 0xFFFFFFFF);
	if (ret)
		goto out;

	ret = len;
out:
	return ret;
}

/* cppcheck-suppress unusedFunction */
int read_stream_1(struct worker *worker)
{
	return read_stream(worker);
}

/* cppcheck-suppress unusedFunction */
int read_stream_QA(struct worker *worker)
{
	int len;

	len = read_stream(worker);
	monitor_send_rxbuffer(worker, len);

	return len;
}

/* cppcheck-suppress unusedFunction */
int prepare_read_stream_QA(struct worker *worker)
{
	int ret = 0;
	struct monitor *monitor = &worker->monitor;
	struct transfer *xfer = &worker->transfer;

	monitor->udp_buffer = malloc(xfer->size + 4);
	if (!monitor->udp_buffer) {
		ret = -ENOMEM;
		goto out;
	}

	monitor->udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (monitor->udp_socket < 0) {
		LOGGING_ERR("%s: socket() failed: %s", __func__,
			strerror(errno));
		ret = -errno;
		goto out_free;
	}
	monitor->udp_addr.sin_family = AF_INET;
	monitor->udp_addr.sin_port = htons(get_param_pc_port());
	if (inet_aton(get_param_pc_host(), &monitor->udp_addr.sin_addr) == 0) {
		LOGGING_ERR("%s: inet_aton(%s) failed", __func__,
			get_param_pc_host());
		ret = -EINVAL;
		goto out_close;
	}

	goto out;

out_close:
	close(monitor->udp_socket);
	monitor->udp_socket = -1;
out_free:
	free(monitor->udp_buffer);
	monitor->udp_buffer = NULL;
out:
	return ret;
}

/* cppcheck-suppress unusedFunction */
int read_stream_SPS_1(struct worker *worker)
{
	int len, ret;
	struct transfer *xfer = &worker->transfer;

	monitor_function_start(worker);
	len = read(xfer->msgbuf, xfer->data, xfer->size);
	if (len < 0) {
		LOGGING_ERR("%s: %s() failed: %s", worker->name, __func__,
			strerror(errno));
		ret = -errno;
		goto out;
	}
	monitor_function_end(worker);

	sps_demo_write_output( &xfer->data[5], 6);

	xfer->rx_packet_id = le16toh(TO_UINT16(xfer->data[1]));

	ret = monitor_rx_timestamp(worker);
	if (ret)
		goto out;
	ret = monitor_packet_identifier(worker, 0xFFFF);
	if (ret)
		goto out;

	ret = len;
out:
	return ret;
}
