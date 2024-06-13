/**
 * @file dump.c
 *
 * Formatting monitored data for dumping
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>

#include "worker.h"
#include "logging.h"
#include "demo.h"
#include "monitor.h"
#include "monitor_server.h"
#include "acmif.h"

static int monitor_dump_count;

static int monitor_dump_buffer(struct worker *worker, struct client *client,
	int version)
{
	int ret;
	struct monitor *monitor = &worker->monitor;
	struct transfer *xfer = &worker->transfer;

	ret = client_send(client,
			"%10s) RX=%u, TX=%u, losses=%u, double=%u, invalid=%u, ",
			worker->name,
			xfer->direction == ACMDRV_BUFF_DESC_BUFF_TYPE_RX ?
				monitor->packet_count : 0,
				xfer->direction == ACMDRV_BUFF_DESC_BUFF_TYPE_TX ?
				monitor->packet_count : 0,
			monitor->packet_lost,
			monitor->packet_duplicated,
			monitor->packet_invalid);
	if (ret)
		goto out;

	if (version == 2)
		ret = client_send(client,
			"missed=%u, max. latency=%d.%03d, ",
			monitor->interval_missed,
			monitor->latency_trace.max_latency / 1000,
			monitor->latency_trace.max_latency % 1000);
	if (ret)
		goto out;

	ret = client_send(client, "TS = [%u, %u, %u]",
		monitor->rx_timestamp_min,
		monitor->rx_timestamp_avg,
		monitor->rx_timestamp_max);
	if (ret)
		goto out;

	if (monitor->timestamp_monitoring_enabled) {
		ret = client_send(client,
			" buf_access_time = [%u, %u, %u]",
			monitor->function_duration_min,
			monitor->function_duration_avg,
			monitor->function_duration_max);
		if (ret)
			goto out;
	}

	ret = client_send(client, "\n");
out:
	return ret;
}

static int monitor_dump_buffer_common(struct client *client)
{
	struct access {
		const char *path;
		unsigned int value;
	};

	int i;
	int ret;
	double loadavg[3];

	struct access drop_frames_cnt_prev[] = {
		[0] = {
			.path = ACMDEV_BASE stringify(ACMDRV_SYSFS_STATUS_GROUP)
				"/drop_frames_cnt_prev_M0",
		},
		[1] = {
			.path = ACMDEV_BASE stringify(ACMDRV_SYSFS_STATUS_GROUP)
				"/drop_frames_cnt_prev_M1",
		},
	};

	struct access tx_frames_prev[] = {
		[0] = {
			.path = ACMDEV_BASE stringify(ACMDRV_SYSFS_STATUS_GROUP)
				"/tx_frames_prev_M0",
		},
		[1] = {
			.path = ACMDEV_BASE stringify(ACMDRV_SYSFS_STATUS_GROUP)
				"/tx_frames_prev_M1",
		}
	};

	struct access rx_frames_prev[] = {
		[0] = {
			.path = ACMDEV_BASE stringify(ACMDRV_SYSFS_STATUS_GROUP)
				"/rx_frames_prev_M0",
		},
		[1] = {
			.path = ACMDEV_BASE stringify(ACMDRV_SYSFS_STATUS_GROUP)
				"/rx_frames_prev_M1",
		}
	};

	ret = client_send(client,
		"-----------------------------%d----------------------------------\n\n",
		monitor_dump_count++);
	if (ret)
		goto out;

	getloadavg(loadavg, 3);
	ret = client_send(client,
		"System load average: %.1f/%.1f/%.1f\n",
		loadavg[0], loadavg[2], loadavg[2]);
	if (ret)
		goto out;

#define READ_SYSFS_ITEM(_item, _i)				\
{								\
	int fd;							\
	int len;						\
	char data[128];						\
								\
	fd = open(_item[(_i)].path, O_RDONLY | O_DSYNC);	\
	if (fd >= 0) {						\
		len = read(fd, data, sizeof(data) - 1);		\
		if (len > 0) {					\
			data[len] = '\0';			\
			_item[(_i)].value			\
				= strtoul(data, NULL, 0);	\
		}						\
		close(fd);					\
	}							\
}

	for (i = 0; i < ACMDRV_BYPASS_MODULES_COUNT; i++) {
		READ_SYSFS_ITEM(drop_frames_cnt_prev, i);
		READ_SYSFS_ITEM(tx_frames_prev, i);
		READ_SYSFS_ITEM(rx_frames_prev,i);
	}

	for (i = 0; i < ACMDRV_BYPASS_MODULES_COUNT; i++) {
		ret = client_send(client,
			"bypass%d: DroppedFramesCounterPrevCycle=%u, TxFramesPrevCycle=%u, RxFramesPrevCycle=%u\n",
			i, drop_frames_cnt_prev[i].value,
			tx_frames_prev[i].value,
			rx_frames_prev[i].value);
		if (ret)
			goto out;
	}
out:
	return ret;
}

static int _monitor_dump(struct client *client, int version)
{
	struct worker *worker;
	int ret;

	ret = monitor_dump_buffer_common(client);
	if (ret)
		goto out;

	STAILQ_FOREACH(worker, &workers, entries) {
		ret = monitor_dump_buffer(worker, client, version);
		if (ret)
			goto out;
	}

	/* indicate end of message */
	ret = client_send(client, "\n\n");

out:
	return ret;
}

int monitor_dump(struct client *client)
{
	return _monitor_dump(client, 1);
}

int monitor_dump2(struct client *client)
{
	return _monitor_dump(client, 2);
}

static int monitor_dump_cycle_single(struct worker *worker,
	struct client *client)
{
	int i;
	int ret;
	struct latency_trace ct;
	struct monitor *monitor = &worker->monitor;
	struct transfer *xfer = &worker->transfer;
	unsigned int last_write;

	pthread_mutex_lock(&monitor->latency_trace.rdlock);
	last_write = (monitor->latency_trace.write - 1) % MONITOR_CYCLE_TRACE_SIZE;
	/* copy entire trace data */
	ct = monitor->latency_trace;
	/* mark all data as read */
	monitor->latency_trace.read = last_write;
	pthread_mutex_unlock(&monitor->latency_trace.rdlock);

	ret = client_send(client, "| Messagebuffer %s", worker->name);
	if (ret)
		goto out;
	ret = client_send(client, "%*.*s|\n",
		68 - (int)strlen(worker->name), 68 - (int)strlen(worker->name),
		"                                                                                         ");
	if (ret)
		goto out;
	ret = client_send(client,
		"+-----------------------+-----------------------+-----------+-----------------------+\n");
	if (ret)
		goto out;
	ret = client_send(client,
		"|      actual time      |    scheduled time     |  latency  |     RX timestamp      |\n");
	if (ret)
		goto out;
	ret = client_send(client,
		"+-----------------------+-----------------------+-----------+-----------------------+\n");
	if (ret)
		goto out;

	for (i = (ct.read + 1) % MONITOR_CYCLE_TRACE_SIZE;
	     i != last_write; i = (i + 1) % MONITOR_CYCLE_TRACE_SIZE) {
		struct timespec *now = &ct.time[i].now;
		struct timespec *next = &ct.time[i].next;
		long long diff;

		diff = (now->tv_sec - next->tv_sec) * NSEC_PER_SEC;
		diff += now->tv_nsec - next->tv_nsec;

		ret = client_send(client,
			"| %11ld.%09ld | %11ld.%09ld | %3lld.%03lldus |",
			now->tv_sec, now->tv_nsec, next->tv_sec, next->tv_nsec,
			diff / NSEC_PER_USEC, diff % NSEC_PER_USEC);
		if (ret)
			goto out;

		/* adapt rx timestamp to full timestamp */
		if (xfer->direction == ACMDRV_BUFF_DESC_BUFF_TYPE_RX) {
			struct timespec ts;
			unsigned int s;

			/* use ns value from timestamp */
			ts.tv_nsec = ct.time[i].rx_timestamp & GENMASK(29, 0);

			/* calc timestamp seconds (it is modulo 4) */
			s = (ct.time[i].rx_timestamp & GENMASK(31, 30)) >> 30;

			ts.tv_sec = (now->tv_sec & GENMASK_ULL(63, 2)) + s;

			if ((ts.tv_sec & GENMASK(1, 0)) < s)
				ts.tv_sec -= 4;

			ret = client_send(client,
				" %11ld.%09ld |", ts.tv_sec, ts.tv_nsec);
		} else {
			ret = client_send(client,
				"                       |");
		}
		if (ret)
			goto out;

#ifdef DEBUG_USING_PKTCNT
		ret = client_send(client, " %u, %u",
			ct.time[write].packet_id, ct.time[write].packet_counter);
		if (ret)
			goto out;

#endif /* DEBUG_USING_PKTCNT */
		ret = client_send(client, "\n");
		if (ret)
			goto out;
	}

	ret = client_send(client,
		"+-----------------------+-----------------------+-----------+-----------------------+\n");
out:
	return ret;
}

int monitor_dump_cycles(struct client *client)
{
	struct worker *worker;
	int ret;

	ret = client_send(client,
		"+-----------------------------------------------------------------------------------+\n");
	if (ret)
		goto out;
	ret = client_send(client,
		"| CYCLE DATA DUMP                                                                   |\n");
	if (ret)
		goto out;
	ret = client_send(client,
		"+-----------------------------------------------------------------------------------+\n");
	if (ret)
		goto out;

	STAILQ_FOREACH(worker, &workers, entries) {
		ret = monitor_dump_cycle_single(worker, client);
		if (ret)
			goto out;
	}

	/* indicate end of message */
	ret = client_send(client, "\n\n");
out:
	return ret;
}

static void printbin16(char *buf, uint16_t val)
{
	int i;

	for (i = 0; i < 16; ++i)
		buf[15 - i] = (val & (1 << i)) ? '1' : '0';
	buf[16] = '\0';
}

static int dump_acm_diagnostic_array(struct client *client, const char *name,
				     const uint32_t *data, size_t count)
{
	int ret;
	int i;

	ret = client_send(client, "|   %-36s: [", name);
	if (ret < 0)
		goto out;
	for (i = 0; i < count - 1; ++i) {
		ret = client_send(client, "%u,", data[i]);
		if (ret < 0)
			goto out;
	}
	ret = client_send(client, "%u]\n", data[count - 1]);

out:
	return ret;
}

static int dump_acm_diagostics(struct client *client,
	const struct acmdrv_diagnostics *data)
{
	int ret;
	char binbuff[ACMDRV_BYPASS_NR_RULES + 1];

	ret = client_send(client, "|   %-36s: %" PRId64 ".%.9" PRId32 "\n",
		"Time", data->timestamp.tv_sec, data->timestamp.tv_nsec);
	if (ret < 0)
		goto out;

	ret = client_send(client, "|   %-36s: %u\n", "Schedule Cycle Counter",
		data->scheduleCycleCounter);
	if (ret < 0)
		goto out;

	ret = client_send(client, "|   %-36s: %u\n", "TX Frames",
		data->txFramesCounter);
	if (ret < 0)
		goto out;

	ret = client_send(client, "|   %-36s: %u\n", "RX Frames",
		data->rxFramesCounter);
	if (ret < 0)
		goto out;

	printbin16(binbuff, data->ingressWindowClosedFlags);
	ret = client_send(client,
		"|   Ingress Window Closed Flags          : %s\n", binbuff);
	if (ret < 0)
		goto out;

	ret = dump_acm_diagnostic_array(client,
		"Ingress Window Closed Counters",
		data->ingressWindowClosedCounter, ACMDRV_BYPASS_NR_RULES);
	if (ret < 0)
		goto out;

	printbin16(binbuff, data->noFrameReceivedFlags);
	ret = client_send(client,
		"|   No Frames Received Flags             : %s\n", binbuff);
	if (ret < 0)
		goto out;

	ret = dump_acm_diagnostic_array(client, "No Frames Received Counters",
		data->noFrameReceivedCounter, ACMDRV_BYPASS_NR_RULES);
	if (ret < 0)
		goto out;

	printbin16(binbuff, data->recoveryFlags);
	ret = client_send(client,
		"|   Recovery Flags                       : %s\n", binbuff);
	if (ret < 0)
		goto out;

	ret = dump_acm_diagnostic_array(client,	"Recovery Counters",
		data->recoveryCounter, ACMDRV_BYPASS_NR_RULES);
	if (ret < 0)
		goto out;

	printbin16(binbuff, data->additionalFilterMismatchFlags);
	ret = client_send(client,
		"|   Additional Filter Mismatch Flags     : %s\n", binbuff);
	if (ret < 0)
		goto out;

	ret = dump_acm_diagnostic_array(client,	"Additional Filter Mismatch Counters",
		data->additionalFilterMismatchCounter, ACMDRV_BYPASS_NR_RULES);

out:
	return ret;
}

static int dump_diag_data_cycle(struct diag_worker *diag_worker,
	struct client *client, int cycle)
{
	int ret = 0;
	int i;

	for (i = 0; i < ACMDRV_BYPASS_MODULES_COUNT; ++i) {
		const struct acmdrv_diagnostics *data = get_diag_worker_data(
			diag_worker);

		ret = client_send(client, "| Cycle %d, Bypass Module %d\n",
			cycle, i);
		if (ret < 0)
			break;
		ret = client_send(client,
			"+------------------------------------------------------------------------------\n");
		if (ret < 0)
			break;

		ret = dump_acm_diagostics(client,
			&data[cycle * ACMDRV_BYPASS_MODULES_COUNT + i]);
		if (ret < 0) {
			LOGGING_ERR("%s: dump_acm_diagostics() failed: %s",
				__func__, strerror(ret));
			break;
		}
		ret = client_send(client,
			"+------------------------------------------------------------------------------\n");
		if (ret < 0)
			break;
	}
	if (ret < 0)
		goto out;

out:
	if (ret < 0)
		LOGGING_ERR("%s: Dumping cycle %u failed: %s",
			__func__, cycle, strerror(ret));

	return ret;
}

int monitor_dump_diag_data(struct client *client,
	struct diag_worker *diag_worker)
{
	int ret, cnt;

	/* send header */
	ret = client_send(client,
		"+------------------------------------------------------------------------------\n");
	if (ret < 0)
		goto out;

	ret = client_send(client, "| DIAGNOSTIC DATA DUMP\n");
	if (ret < 0)
		goto out;

	ret = client_send(client,
		"+------------------------------------------------------------------------------\n");
	if (ret < 0)
		goto out;

	for (cnt = 0; cnt < get_diag_worker_cycle(diag_worker); ++cnt) {
		ret = dump_diag_data_cycle(diag_worker, client, cnt);
		if (ret) {
			LOGGING_ERR("%s: Dumping cycle %u failed: %s",
				__func__, cnt, strerror(ret));

			break;
		}
	}
	if (ret < 0)
		goto out;

	/* mark end of message */
	ret = client_send(client, "\n\n");
out:
	return ret;
}
