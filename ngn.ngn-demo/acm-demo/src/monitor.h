/**
 * @file monitor.h
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
#ifndef MONITOR_H_
#define MONITOR_H_

#include <time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <signal.h>

#include "demo.h"
#include "tracer.h"
#include "worker.h"
#include "configuration.h"
#include "logging.h"

void monitor_send_rxbuffer(struct worker *worker, int rx_msg_len);

static inline int64_t calcdiff_ns(const struct timespec *after,
	const struct timespec *before)
{
	int64_t diff;

	diff = NSEC_PER_SEC * (int64_t)((int) after->tv_sec -
		(int) before->tv_sec);
	diff += ((int) after->tv_nsec - (int) before->tv_nsec);
	return diff;
}

static inline uint32_t calc_cum_avg(uint32_t oldavg, uint32_t val, uint32_t n)
{
	uint64_t old = oldavg;

	/* restart after wrap around of n */
	if (n == 0  && oldavg != 0)
		return (oldavg + val) / 2;

	/* avoid division by zero at wrap around of n */
	if (n == 0xFFFFFFFF)
		n--;

	return (uint32_t)((old * n + val) / (n + 1));
}

static inline int monitor_trace_latency(struct worker *worker,
	const struct timespec *now)
{
	uint32_t lat;
	struct latency_trace *lt = &worker->monitor.latency_trace;
	unsigned int idx = lt->write % MONITOR_CYCLE_TRACE_SIZE;

//	pthread_mutex_lock(&lt->lock);
//	if (idx != lt->read % MONITOR_CYCLE_TRACE_SIZE) {
		lt->time[idx].now = *now;
		lt->time[idx].next = worker->next;
		lt->time[idx].rx_timestamp =
			worker->transfer.last_rx_timestamp;
#ifdef DEBUG_USING_PKTCNT
		lt->time[idx].packet_counter = worker->monitor.packet_count;
		lt->time[idx].packet_id =
			worker->transfer.direction == ACMDRV_BUFF_DESC_BUFF_TYPE_RX ?
				worker->transfer.rx_packet_id :
				worker->transfer.packet_id;
#endif /* DEBUG_USING_PKTCNT */


		++lt->write;
//	}

//	pthread_mutex_unlock(&lt->lock);

	lat = calcdiff_ns(now, &worker->next);
	if (lat > lt->max_latency)
		lt->max_latency = lat;

	if (lt->lat_break_at && lat >= lt->lat_break_at) {
		tracemark("%s: Latency exceeded: %u > %u",
			worker->config->buffer_name, lat,
			lt->lat_break_at);
		tracemark("%s: now = %ld.%09ld",
			worker->config->buffer_name,
			now->tv_sec, now->tv_nsec);
		tracemark("%s: next = %ld.%09ld",
			worker->config->buffer_name,
			worker->next.tv_sec, worker->next.tv_nsec);
		tracemark("%s: packet_count = %u, packet_id = %u",
			worker->config->buffer_name,
			worker->monitor.packet_count,
			worker->transfer.packet_id);

//		pthread_mutex_unlock(&lt->lock);
		return -EIO;
	}

	return 0;
}

static inline void monitor_function_start(struct worker *worker)
{
	struct monitor *monitor = &worker->monitor;

	monitor->function_duration_valid = false;
	if (monitor->timestamp_monitoring_enabled)
		clock_gettime(worker->clock_id, &monitor->function_start);
}

static inline void monitor_function_end(struct worker *worker)
{
	struct monitor *monitor = &worker->monitor;

	if (monitor->timestamp_monitoring_enabled) {
		clock_gettime(worker->clock_id, &monitor->function_end);
		monitor->function_duration_valid = true;
	}
}

static inline int monitor_check_durationlimit(struct worker *worker,
					       uint32_t diff)
{
#ifdef FTRACE_SUPPORT_ENABLED
	struct monitor *monitor = &worker->monitor;
	uint32_t limit = monitor->function_duration_limit;

	if (limit && (diff > limit)) {
		tracemark("%s: Duration exceeded: %u > %u",
			worker->config->buffer_name, diff, limit);
		return -EIO;
	}
#endif /* FTRACE_SUPPORT_ENABLED */
	return 0;
}

static inline int monitor_function_duration(struct worker *worker)
{
	struct monitor *monitor = &worker->monitor;

	/*
	 * cast down to uint32_t to speed up (we still can store more than 4s
	 * time difference
	 */

	if (monitor->function_duration_valid) {
		uint32_t diff;

		diff = calcdiff_ns(&monitor->function_end,
			&monitor->function_start);

		if (diff < monitor->function_duration_min)
			monitor->function_duration_min = diff;

		if (monitor->function_duration_max < diff)
			monitor->function_duration_max = diff;

		monitor->function_duration_avg =
			calc_cum_avg(monitor->function_duration_avg,
			diff,
			monitor->function_duration_count);

		monitor->function_duration_count++;

		return monitor_check_durationlimit(worker, diff);
	}
	return 0;

}

static inline bool monitor_max_cycle_reached(struct worker *worker)
{
	return worker->max_cycle
		&& (worker->monitor.packet_count >= worker->max_cycle);
}

static inline void monitor_packet_transferred(struct worker *worker)
{
	++worker->monitor.packet_count;
}

static inline int monitor_rx_timestamp(struct worker *worker)
{
	uint32_t timestamp;
	struct transfer *transfer = &worker->transfer;
	struct monitor *monitor = &worker->monitor;

	if (!transfer->has_timestamp)
		return 0;

	timestamp = be32toh(TO_UINT32(transfer->data[worker->config->size]));
	if (timestamp == transfer->last_rx_timestamp) {
		/*
		 * nothing received, thus reading the same
		 * packet again
		 */
		++monitor->packet_lost;
		if (monitor->break_on_loss &&
		    (monitor->packet_lost >= monitor->break_on_loss)) {
			tracemark("%s: Lost packets exceeded: %u >= %u",
				worker->config->buffer_name,
				monitor->packet_lost, monitor->break_on_loss);
			tracemark("%s: packet_count = %u, packet_id = %u",
				worker->config->buffer_name,
				worker->monitor.packet_count,
				worker->transfer.packet_id);
			LOGGING_ERR("%s: Lost packets exceeded: %u >= %u",
				worker->config->buffer_name,
				monitor->packet_lost, monitor->break_on_loss);
			return -EIO;
		}

		return 0;
	}

	transfer->last_rx_timestamp = timestamp;

	timestamp &= 0x3FFFFFFF; /* ignore seconds */
	timestamp %= worker->interval_ns;

	if (timestamp > monitor->rx_timestamp_max)
		monitor->rx_timestamp_max = timestamp;
	if (timestamp < monitor->rx_timestamp_min)
		monitor->rx_timestamp_min = timestamp;

	monitor->rx_timestamp_avg = calc_cum_avg(
		monitor->rx_timestamp_avg,
		timestamp, monitor->rx_timestamp_count++);

	return 0;
}

static inline int monitor_packet_identifier(struct worker *worker,
	uint32_t mask)
{
	struct transfer *transfer = &worker->transfer;
	struct monitor *monitor = &worker->monitor;

	/* we usually receive the frame sent last cycle */
	uint32_t target_id = transfer->packet_id - transfer->packet_id_offs;
	target_id &= mask;

	switch (target_id - (transfer->rx_packet_id & mask)) {
	case 0:
		monitor_packet_transferred(worker);
		break;
	case 1:
		++monitor->packet_duplicated;

		if (monitor->break_on_double &&
		   (monitor->packet_duplicated >= monitor->break_on_double)) {
			tracemark("%s: Double packets exceeded: %u >= %u",
				worker->config->buffer_name,
				monitor->packet_duplicated,
				monitor->break_on_double);
			tracemark("%s: packet_count = %u, packet_id = %u",
				worker->config->buffer_name,
				worker->monitor.packet_count,
				worker->transfer.rx_packet_id);
			LOGGING_ERR("%s: Double packets exceeded: %u >= %u",
				worker->config->buffer_name,
				monitor->packet_duplicated,
				monitor->break_on_double);
			return -EIO;
		}
		break;
	default:
		++monitor->packet_invalid;

		if (monitor->break_on_invalid &&
		   (monitor->packet_invalid >= monitor->break_on_invalid)) {
			tracemark("%s: Invalid packets exceeded: %u >= %u",
				worker->config->buffer_name,
				monitor->packet_invalid,
				monitor->break_on_invalid);
			tracemark("%s: packet_count = %u, packet_id = %u",
				worker->config->buffer_name,
				worker->monitor.packet_count,
				worker->transfer.rx_packet_id);
			LOGGING_ERR("%s: Invalid packets exceeded: %u >= %u",
				worker->config->buffer_name,
				monitor->packet_invalid,
				monitor->break_on_invalid);
			return -EIO;
		}
	}

	return 0;
}

static inline int monitor_interval_exceeded(struct worker *worker,
	const struct timespec *now)
{
	struct monitor *monitor = &worker->monitor;

	++monitor->interval_missed;

	if (monitor->break_on_missed &&
	   (monitor->interval_missed >= monitor->break_on_missed)) {
		tracemark("%s: Missed packets exceeded: %u >= %u, now=%ld.%09ld, next=%ld.%09ld",
			worker->config->buffer_name,
			monitor->interval_missed,
			monitor->break_on_missed,
			now->tv_sec, now->tv_nsec, worker->next.tv_sec,
			worker->next.tv_nsec);
		return -EIO;
	}

	return 0;
}

int monitor_dump_diag(struct client *client, const char *command);

#endif /* MONITOR_H_ */
