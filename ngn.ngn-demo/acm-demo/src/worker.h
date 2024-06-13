/**
 * @file worker.h
 *
 * Worker function framework
 *
 * These functions are linked in dynamically depending on the configuration.
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#ifndef WORKER_H_
#define WORKER_H_

#include <stdint.h>
#include <stdbool.h>
#include <sys/queue.h>
#include <pthread.h>
#include <netinet/in.h>
#include <linux/acm/acmdrv.h>

struct worker {
	char *name;
	struct configuration_entry *config;

	pthread_t thread;
	pthread_attr_t attr;
	bool is_running;

	clockid_t clock_id;
	uint32_t interval_ns;	/* interval from config in ns */
	struct timespec start;
	struct timespec next;
	uint32_t max_cycle;

#define TO_ASCII(val)	(((val) & 0x3F) + ' ')
#define TO_UINT16(var)	(*((uint16_t *)&(var)))
#define TO_UINT32(var)	(*((uint32_t *)&(var)))

	struct transfer {
		int msgbuf;
		uint8_t *data;
		size_t size;

		uint32_t packet_id; /* packet identifier base on time offset */
		off_t packet_id_offs; /* RX packet id offset */
		uint32_t rx_packet_id; /* packet identifier read from packet */

		bool has_timestamp;
		uint32_t last_rx_timestamp;

		enum acmdrv_buff_desc_type direction;
	} transfer;

	struct monitor {
		/* worker function duration monitoring */
		bool timestamp_monitoring_enabled;
		struct timespec function_start;
		struct timespec function_end;
		bool function_duration_valid;
		unsigned int function_duration_min;
		unsigned int function_duration_max;
		unsigned int function_duration_avg;
		unsigned int function_duration_count;
		unsigned int function_duration_limit;

		unsigned int packet_count; /* number of transferred packets */
		unsigned int packet_lost; /* number of lost packets */
		unsigned int packet_duplicated;
		unsigned int packet_invalid;

		unsigned int interval_missed;

		uint32_t rx_timestamp_min;
		uint32_t rx_timestamp_max;
		uint32_t rx_timestamp_avg;
		uint32_t rx_timestamp_count;

#define MONITOR_CYCLE_TRACE_SIZE	16
		struct latency_trace {
			pthread_mutex_t rdlock;
//			pthread_mutexattr_t attr;
			uint32_t max_latency;
			uint32_t lat_break_at;
			unsigned int write;
			unsigned int read;
			struct {
				struct timespec now;
				struct timespec next;
				uint32_t rx_timestamp;
#ifdef DEBUG_USING_PKTCNT
				uint32_t packet_counter;
				uint32_t packet_id;
#endif /* DEBUG_USING_PKTCNT */
			} time[MONITOR_CYCLE_TRACE_SIZE];
		} latency_trace;

		int break_on_loss;
		int break_on_double;
		int break_on_invalid;
		int break_on_missed;

		uint8_t *udp_buffer;
		int udp_socket;
		struct sockaddr_in udp_addr;
	} monitor;

	STAILQ_ENTRY(worker) entries;
};

STAILQ_HEAD(worker_list, worker);
extern struct worker_list workers;

struct configuration_list;
struct monitor;

int create_workers(struct configuration_list *configurations);
void free_workers(void);
int start_workers(void);
void stop_workers(void);

struct monitor *worker_getmonitor(struct worker *worker);
void worker_setmonitor(struct worker *worker, struct monitor *monitor);

int get_num_workers();

struct diag_worker;
struct client;
struct diag_worker *create_diag_worker(struct client *client, uint32_t offset,
	uint32_t mult, uint32_t cnt);
void destroy_diag_worker(struct diag_worker *diag_worker);
int start_diag_worker(struct diag_worker *diag_worker);
const struct acmdrv_diagnostics *get_diag_worker_data(const struct diag_worker *);
uint32_t get_diag_worker_cycle(const struct diag_worker *dw);
int wait_diag_worker_finished(const struct diag_worker *dw);
void stop_diag_workers(void);


#endif /* WORKER_H_ */
