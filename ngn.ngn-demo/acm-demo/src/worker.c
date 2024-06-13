/**
 * @file worker.c
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
#define _GNU_SOURCE
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <limits.h>

#include "worker.h"
#include "configuration.h"
#include "logging.h"
#include "demo.h"
#include "messagebuffer.h"
#include "monitor.h"
#include "acmif.h"
#include "parser.h"

#define DEVBASE	"/dev/"

struct worker_list workers = STAILQ_HEAD_INITIALIZER(workers);

/* all workers share a common basic start time */
static struct timespec starttime;

int get_num_workers(void)
{
	struct worker *worker;
	int i = 0;

	STAILQ_FOREACH(worker, &workers, entries)
		++i;

	return i;
}

int worker_setaffinity(struct worker *worker, const cpu_set_t *affinity)
{
	int ret;

	LOGGING_DEBUG("%s(%s, %d)", __func__, worker->name,
		cpu_set_to_int(affinity));
	ret = pthread_attr_setaffinity_np(&worker->attr, sizeof(cpu_set_t),
					  affinity);
	if (ret) {
		LOGGING_ERR("Cannot set CPU affinity for worker %s: %s",
			worker->name, strerror(ret));
	}

	return -ret;
}

int worker_setschedpolicy(struct worker *worker, int policy)
{
	int ret;

	LOGGING_DEBUG("%s(%s, %d)", __func__, worker->name, policy);
	ret = pthread_attr_setschedpolicy(&worker->attr, policy);
	if (ret)
		LOGGING_ERR("Cannot set schedule policy for worker %s: %s",
			worker->name, strerror(ret));

	return -ret;
}

int worker_setschedparam(struct worker *worker, struct sched_param *param)
{
	int ret;

	LOGGING_DEBUG("%s(%s, %d)", __func__, worker->name,
		param->sched_priority);
	ret = pthread_attr_setschedparam(&worker->attr, param);
	if (ret)
		LOGGING_ERR("Cannot set schedule priority for worker %s: %s",
			worker->name, strerror(ret));
	return -ret;
}

static struct worker *create_worker(struct configuration_entry *config)
{
	int ret;
	struct worker *worker;
	struct sched_param param;
	const int policy = SCHED_FIFO;
	char *devicename;
	cpu_set_t affinity;
	struct latency_trace *lt;

	LOGGING_DEBUG("%s for config %d (%s)", __func__, config->index,
		config->buffer_name);

	/* set default priority */
	param.sched_priority = (sched_get_priority_max(policy) +
				sched_get_priority_min(policy)) / 3 * 2;


	worker = calloc(1, sizeof(*worker));
	if (!worker) {
		LOGGING_ERR("%s: %s", __func__, strerror(ENOMEM));
		return NULL;
	}

	lt = &worker->monitor.latency_trace;
	worker->name = strdup(config->buffer_name);
	worker->is_running = false;
	worker->clock_id = CLOCK_REALTIME;
	worker->interval_ns = NSEC_PER_USEC * config->period;
	worker->max_cycle = get_param_max_frame_counter();
	worker->config = config;

	worker->transfer.packet_id_offs = get_param_rxoffset();
	worker->transfer.has_timestamp =
		acmdrv_buff_desc_timestamp_read(&config->msgbuf->desc);
	worker->transfer.direction =
		acmdrv_buff_desc_type_read(&config->msgbuf->desc);

	worker->monitor.udp_buffer = NULL;
	worker->monitor.udp_socket = -1;

	worker->monitor.timestamp_monitoring_enabled =
		worker->config->use_timestamp;
	worker->monitor.function_duration_min = UINT_MAX;
	worker->monitor.function_duration_max = 0;
	worker->monitor.function_duration_avg = 0;
	worker->monitor.function_duration_count = 0;
	worker->monitor.function_duration_limit =
		get_param_tracelimit() * NSEC_PER_USEC;

	worker->monitor.rx_timestamp_min =
		worker->transfer.direction == ACMDRV_BUFF_DESC_BUFF_TYPE_RX ?
			UINT_MAX : 0;
	worker->monitor.rx_timestamp_max = 0;
	worker->monitor.rx_timestamp_avg = 0;
	worker->monitor.rx_timestamp_count = 0;

	worker->monitor.break_on_loss = get_param_break_on_loss();
	worker->monitor.break_on_double = get_param_break_on_double();
	worker->monitor.break_on_invalid = get_param_break_on_invalid();
	worker->monitor.break_on_missed = get_param_break_on_missed();

//	pthread_mutexattr_init(&worker->monitor.latency_trace.attr);
//	pthread_mutexattr_setprotocol(&worker->monitor.latency_trace.attr,
//		PTHREAD_PRIO_INHERIT);
//	pthread_mutex_init(&worker->monitor.latency_trace.lock,
//		&worker->monitor.latency_trace.attr);
	pthread_mutex_init(&lt->rdlock, NULL);
	lt->lat_break_at = get_param_lat_break();
	lt->max_latency = 0;
	lt->read = MONITOR_CYCLE_TRACE_SIZE - 1;
	lt->write = 0;

	asprintf(&devicename, "%s%s", DEVBASE, worker->name);
	if (!devicename)
		goto out_free_worker;

	worker->transfer.msgbuf = open(devicename,
		worker->transfer.direction == ACMDRV_BUFF_DESC_BUFF_TYPE_RX ?
			O_RDONLY : O_WRONLY);
	LOGGING_DEBUG("%s: open(%s): %d", worker->name, devicename,
		worker->transfer.msgbuf);
	free(devicename);

	if (worker->transfer.msgbuf == -1) {
		LOGGING_ERR("Cannot open message buffer device for worker %s: %s",
			worker->name, strerror(errno));
		goto out_free_worker;
	}

	worker->transfer.size = acmif_msgbuf_size(&config->msgbuf->desc);
	if (worker->transfer.size == 0)
		goto out_free_worker;

	worker->transfer.data = malloc(worker->transfer.size);
	if (!worker->transfer.data)
		goto out_close;

	/* set reasonable defaults */
	ret = pthread_attr_init(&worker->attr);
	if (ret) {
		LOGGING_ERR("Cannot initialize thread attribute for worker %s: %s",
			worker->name, strerror(ret));
		goto out_free_data;
	}

	ret = pthread_attr_setinheritsched(&worker->attr,
					   PTHREAD_EXPLICIT_SCHED);
	if (ret) {
		LOGGING_ERR("pthread_attr_setinheritsched failed: %d", ret);
		goto out_attr_destroy;
	}

	ret = worker_setschedpolicy(worker, policy);
	if (ret)
		goto out_attr_destroy;

	ret = worker_setschedparam(worker, &param);
	if (ret)
		goto out_attr_destroy;

	if (get_param_distribute()) {
		int cpus;
		int chosen_cpu;
		int i;
		cpu_set_t cpu_set;

		CPU_ZERO(&cpu_set);
		sched_getaffinity(0, sizeof(cpu_set), &cpu_set);
		cpus = CPU_COUNT(&cpu_set);

		chosen_cpu = worker->config->index % cpus;
		for (i = 0; i < 8 * sizeof(int); ++i) {
			if (!CPU_ISSET(i, &cpu_set))
				continue;

			if (chosen_cpu == 0) {
				CPU_ZERO(&affinity);
				CPU_SET(i, &affinity);
				break;
			}
			--chosen_cpu;
		}
		ret = worker_setaffinity(worker, &affinity);
	} else
		ret = worker_setaffinity(worker, get_param_cpuset());
	if (ret)
		goto out_attr_destroy;

	ret = pthread_attr_setstacksize(&worker->attr, 0x40000);
	if (ret) {
		LOGGING_ERR("pthread_attr_setstacksize failed: %d", ret);
		goto out_attr_destroy;
	}

	/* interconnect configuration and worker */
	configuration_set_worker(config, worker);

	return worker;

out_attr_destroy:
	pthread_attr_destroy(&worker->attr);
out_free_data:
	free(worker->transfer.data);
out_close:
	LOGGING_DEBUG("%s: Closing %d", worker->name, worker->transfer.msgbuf);
	close(worker->transfer.msgbuf);
out_free_worker:
	free(worker->name);
	free(worker);
	return NULL;
}

static void *worker_func(void *data)
{
	struct timespec now;
	struct worker *worker = data;

	pthread_setname_np(pthread_self(), worker->name);
	worker->is_running = true;

	worker->next = worker->start;

	if (worker->transfer.direction == ACMDRV_BUFF_DESC_BUFF_TYPE_RX) {
		int i;
		/* dummy read to get eventually old data from buffer */
		for (i = 0; i < 10; ++i)
			read(worker->transfer.msgbuf, worker->transfer.data,
				worker->transfer.size);
	}

	/* sync to next interval start */
	do {
		if (clock_gettime(worker->clock_id, &now) < 0) {
			LOGGING_ERR("%s: clock_gettime() failed: %s",
				worker->name, strerror(errno));
			force_stop();
			while (worker->is_running) {
				usleep(10000);
			}
			pthread_exit(NULL);
		}
		tsinc(&worker->next, worker->interval_ns);
	} while (tsgreater(&now, &worker->next));

	/*
	 * add one additional interval to make sure to sync on the
	 * next interval start with clock_nanosleep below
	 */
	tsinc(&worker->next, worker->interval_ns);
	/*
	 * add some intervals for receive workers to give the corresponding
	 * senders a chance to provide valid data
	 */
	if (worker->transfer.direction == ACMDRV_BUFF_DESC_BUFF_TYPE_RX) {
		tsinc(&worker->next, worker->interval_ns);
		tsinc(&worker->next, worker->interval_ns);
		tsinc(&worker->next, worker->interval_ns);
	}

	while (worker->is_running) {
		int ret, ret1;

		ret = clock_nanosleep(worker->clock_id, TIMER_ABSTIME,
				      &worker->next, NULL);
		ret1 = clock_gettime(worker->clock_id, &now);
		if (ret < 0) {
			LOGGING_ERR("%s: clock_nanosleep() failed: %s",
				worker->name, strerror(errno));
			force_stop();
		}

		/* get time to trace cycle accuracy */
		if (ret1 < 0) {
			LOGGING_ERR("%s: clock_gettime() failed: %s",
				worker->name, strerror(errno));
			force_stop();
		}

		/* derive packet counter from time */
		worker->transfer.packet_id = (worker->next.tv_sec * NSEC_PER_SEC
			+ worker->next.tv_nsec) / worker->interval_ns;

		ret = (*(worker->config->function))(worker);

		if (monitor_function_duration(worker))
			force_stop();
		if (monitor_trace_latency(worker, &now))
			force_stop();
		if (monitor_max_cycle_reached(worker))
			force_stop();

		/* ignore ENODATA */
		if ((ret < 0) && (ret != -ENODATA)) {
			LOGGING_ERR("%s: worker function failed: %s",
				worker->name, strerror(-ret));
			force_stop();
		}

		/* add at least one interval */
		tsinc(&worker->next, worker->interval_ns);

		ret = clock_gettime(worker->clock_id, &now);
		if (ret < 0) {
			LOGGING_ERR("%s: clock_gettime() failed: %s",
				worker->name, strerror(errno));
			force_stop();
		}

		while (tsgreater(&now, &worker->next)) {
			if (monitor_interval_exceeded(worker, &now))
				force_stop();
			tsinc(&worker->next, worker->interval_ns);
		}
	}

	LOGGING_DEBUG("Exiting %s worker", worker->name);
	pthread_exit(NULL);
}

static int start_worker(struct worker *worker)
{
	int ret;

	/*
	 * initialize worker specific start time by adding the
	 * worker specific offset
	 */
	worker->start = starttime;
	worker->start.tv_nsec += worker->config->time_offs * NSEC_PER_USEC;
	tsnorm(&worker->start);

	LOGGING_DEBUG("%s for %s at %d.%09d, interval=%" PRIu64 "us",
		__func__, worker->name,
		worker->start.tv_sec, worker->start.tv_nsec,
			worker->interval_ns / NSEC_PER_USEC);
	/* prepare worker function, if applicable */
	if (worker->config->prepare_function) {
		ret = (*(worker->config->prepare_function))(worker);
		if (ret != 0)
			goto out;
	}
	/* error codes are negative */
	ret = -pthread_create(&worker->thread, &worker->attr, worker_func,
			      worker);
out:
	if (ret)
		LOGGING_ERR("Cannot start worker %s: %s",
			worker->name, strerror(ret));
	return ret;
}

static int _calculate_starttime(struct timespec *time, uint32_t interval_us,
	unsigned int delay)
{
	int ret;
	uint32_t rem;
	const uint64_t cycns = interval_us * NSEC_PER_USEC;

	ret = clock_gettime(CLOCK_REALTIME, time);
	if (ret < 0) {
		LOGGING_ERR("clock_gettime(CLOCK_REALTIME) failed: %s",
			strerror(errno));
		return -errno;
	}

	/* add <delay> cycles to get at least one cycle start offset */
	time->tv_sec += (delay *interval_us) / USEC_PER_SEC;
	time->tv_nsec += ((delay *interval_us) % USEC_PER_SEC) * NSEC_PER_USEC;
	tsnorm(time);

	rem = (((time->tv_sec % cycns) * (NSEC_PER_SEC % cycns)) % cycns +
		(time->tv_nsec % cycns)) % cycns;

	time->tv_nsec -= rem % NSEC_PER_SEC;
	while (time->tv_nsec < 0) {
		time->tv_nsec += NSEC_PER_SEC;
		--time->tv_sec;
	}
	time->tv_sec -= rem / NSEC_PER_SEC;

	return 0;
}

static int calculate_starttime(struct timespec *time)
{
	return _calculate_starttime(time, get_param_hw_cycle_us(), 16);
}


int start_workers(void)
{
	int ret;
	struct worker *worker;

	ret = calculate_starttime(&starttime);
	if (ret)
		return ret;

	STAILQ_FOREACH(worker, &workers, entries)
	{
		ret = start_worker(worker);
		if (ret)
			return ret;
	}

	return 0;
}

static void stop_worker(struct worker* worker)
{
	void *thread_ret;

	worker->is_running = false;
	pthread_join(worker->thread, &thread_ret);

	if (worker->monitor.udp_socket > 0)
		close(worker->monitor.udp_socket);
	free(worker->monitor.udp_buffer);
}

void stop_workers(void)
{
	struct worker *worker;

	STAILQ_FOREACH(worker, &workers, entries)
		stop_worker(worker);
}

int create_workers(struct configuration_list *configurations)
{
	int ret = 0;
	struct configuration_entry *config;
	struct worker *worker;

	STAILQ_FOREACH(config, configurations, entries)
	{
		worker = create_worker(config);
		if (!worker) {
			ret = -ENOMEM;
			goto out;
		}

		STAILQ_INSERT_TAIL(&workers, worker, entries);
	}
out:
	return ret;
}

static void free_worker(struct worker *worker)
{
	if (worker->is_running)
		stop_worker(worker);

	configuration_set_worker(worker->config, NULL);
	pthread_attr_destroy(&worker->attr);
	free(worker->transfer.data);
	close(worker->transfer.msgbuf);
	pthread_mutex_destroy(&worker->monitor.latency_trace.rdlock);
//	pthread_mutexattr_destroy(&worker->monitor.latency_trace.attr);
	LOGGING_DEBUG("%s: Closing %d", worker->name, worker->transfer.msgbuf);
	free(worker->name);
	free(worker);
}

void free_workers(void)
{
	struct worker *worker;

	STAILQ_FOREACH(worker, &workers, entries)
	{
		STAILQ_REMOVE(&workers, worker, worker, entries);
		free_worker(worker);
	}
}

struct diag_worker {
	char *name;

	pthread_t thread;
	pthread_attr_t attr;
	bool is_running;

	clockid_t clock_id;
	uint32_t interval_ns;
	struct timespec start;
	struct timespec next;

	uint32_t max_cycle;
	struct acmdrv_diagnostics *data;

	struct client *client;

	STAILQ_ENTRY(diag_worker) entries;
};

static unsigned int diag_count = 0;
static STAILQ_HEAD(diag_worker_list, diag_worker) diag_workers =
	STAILQ_HEAD_INITIALIZER(diag_workers);

const struct acmdrv_diagnostics *get_diag_worker_data(
	const struct diag_worker *dw)
{
	return dw->data;
}

uint32_t get_diag_worker_cycle(const struct diag_worker *dw)
{
	return dw->max_cycle;
}

int wait_diag_worker_finished(const struct diag_worker *dw)
{
	int ret;
	int thread_ret;

	ret = pthread_join(dw->thread, (void **)&thread_ret);
	if (ret)
		return -ret;

	return thread_ret;
}

/**
 * @brief Create a diag_worker to read diagnostic data synchronously
 */
struct diag_worker *create_diag_worker(struct client *client, uint32_t offset,
	uint32_t mult, uint32_t cnt)
{
	int ret;
	struct sched_param param;
	const int policy = SCHED_FIFO;
	struct diag_worker *diag_worker = calloc(1, sizeof(*diag_worker));

	if (!diag_worker)
		return NULL;

	/* set default priority */
	param.sched_priority = (sched_get_priority_max(policy) +
				sched_get_priority_min(policy)) / 3;

	if (asprintf(&diag_worker->name, "diagnostic%d", diag_count++) < 0)
		goto out_free;

	diag_worker->data = calloc(cnt * ACMDRV_BYPASS_MODULES_COUNT,
		sizeof(struct acmdrv_diagnostics));
	if (!diag_worker->data)
		goto out_free_name;

	diag_worker->is_running = false;
	diag_worker->clock_id = CLOCK_REALTIME;
	diag_worker->max_cycle = cnt;
	diag_worker->interval_ns = NSEC_PER_USEC * get_param_hw_cycle_us() *
		mult;
	diag_worker->client = client;

	/* set reasonable defaults */
	ret = pthread_attr_init(&diag_worker->attr);
	if (ret) {
		LOGGING_ERR("Cannot initialize thread attribute for diag_worker %s: %s",
			diag_worker->name, strerror(ret));
		goto out_free_data;
	}

	ret = pthread_attr_setinheritsched(&diag_worker->attr,
		PTHREAD_EXPLICIT_SCHED);
	if (ret) {
		LOGGING_ERR("pthread_attr_setinheritsched failed: %d", ret);
		goto out_attr_destroy;
	}

	ret = pthread_attr_setschedpolicy(&diag_worker->attr, policy);
	if (ret)
		goto out_attr_destroy;

	ret = pthread_attr_setschedparam(&diag_worker->attr, &param);
	if (ret)
		goto out_attr_destroy;

	ret = pthread_attr_setaffinity_np(&diag_worker->attr, sizeof(cpu_set_t),
		get_param_cpuset());
	if (ret)
		goto out_attr_destroy;

	ret = pthread_attr_setstacksize(&diag_worker->attr, 0x40000);
	if (ret) {
		LOGGING_ERR("pthread_attr_setstacksize failed: %d", ret);
		goto out_attr_destroy;
	}

	ret = _calculate_starttime(&diag_worker->start,
		get_param_hw_cycle_us() * mult, 0);
	if (ret)
		goto out_free_name;
	diag_worker->start.tv_nsec += offset * NSEC_PER_USEC;
	tsnorm(&diag_worker->start);
	diag_worker->max_cycle = cnt;

	LOGGING_DEBUG("%s for %s at %d.%09d, interval=%" PRIu64 "us",
		__func__, diag_worker->name,
		diag_worker->start.tv_sec, diag_worker->start.tv_nsec,
		diag_worker->interval_ns / NSEC_PER_USEC);

	STAILQ_INSERT_TAIL(&diag_workers, diag_worker, entries);
	return diag_worker;

out_attr_destroy:
	pthread_attr_destroy(&diag_worker->attr);

out_free_data:
	free(diag_worker->data);

out_free_name:
	free(diag_worker->name);

out_free:
	free(diag_worker);
	return NULL;

}

void destroy_diag_worker(struct diag_worker *diag_worker)
{
	if (!diag_worker)
		return;

	STAILQ_REMOVE(&diag_workers, diag_worker, diag_worker, entries);
	pthread_attr_destroy(&diag_worker->attr);
	free(diag_worker->data);
	free(diag_worker->name);
	free(diag_worker);
}

static void *diag_func(void *data)
{
	intptr_t ret = 0;
	struct timespec now;
	struct diag_worker *diag_worker = data;
	unsigned int i = 0;
	int fd0, fd1;

	pthread_setname_np(pthread_self(), diag_worker->name);
	diag_worker->is_running = true;

	diag_worker->next = diag_worker->start;
	fd0 = open(ACMDEV_BASE "diag/diagnostics_M0", O_RDONLY | O_DSYNC);
	if (fd0 < 0) {
		ret = -errno;
		LOGGING_ERR("%s: Opening %s failed: %d", __func__,
			ACMDEV_BASE "diag/diagnostics_M0", ret);
		goto done;
	}
	fd1 = open(ACMDEV_BASE "diag/diagnostics_M1", O_RDONLY | O_DSYNC);
	if (fd1 < 0) {
		ret = -errno;
		LOGGING_ERR("%s: Opening %s failed: %d", __func__,
			ACMDEV_BASE "diag/diagnostics_M1", ret);
		close(fd0);
		goto done;
	}
	/* sync to next interval start */
	do {
		if (clock_gettime(diag_worker->clock_id, &now) < 0) {
			LOGGING_ERR("%s: clock_gettime() failed: %s",
				diag_worker->name, strerror(errno));
			goto done;
		}
		tsinc(&diag_worker->next, diag_worker->interval_ns);
	} while (tsgreater(&now, &diag_worker->next));

	/*
	 * add one additional interval to make sure to sync on the
	 * next interval start with clock_nanosleep below
	 */
	tsinc(&diag_worker->next, diag_worker->interval_ns);

	while (diag_worker->is_running && (i < diag_worker->max_cycle)) {
		ret = clock_nanosleep(diag_worker->clock_id, TIMER_ABSTIME,
				      &diag_worker->next, NULL);
		if (ret < 0) {
			LOGGING_ERR("%s: clock_nanosleep() failed: %s",
				diag_worker->name, strerror(errno));
			goto done;
		}

		ret = pread(fd0,
			&diag_worker->data[i * ACMDRV_BYPASS_MODULES_COUNT],
			sizeof(struct acmdrv_diagnostics), 0);
		if (ret < 0) {
			ret = -errno;
			goto done_close;
		}
		ret = pread(fd1,
			&diag_worker->data[i * ACMDRV_BYPASS_MODULES_COUNT + 1],
			sizeof(struct acmdrv_diagnostics), 0);
		if (ret < 0) {
			ret = -errno;
			goto done_close;
		}

		/* add at least one interval */
		tsinc(&diag_worker->next, diag_worker->interval_ns);
		ret = clock_gettime(diag_worker->clock_id, &now);
		if (ret < 0) {
			LOGGING_ERR("%s: clock_gettime() failed: %s",
				diag_worker->name, strerror(errno));
			goto done;
		}

		while (tsgreater(&now, &diag_worker->next))
			tsinc(&diag_worker->next, diag_worker->interval_ns);

		i++;
	}
done_close:
	close(fd0);
	close(fd1);
done:
	pthread_exit((void *)ret);
}

int start_diag_worker(struct diag_worker *diag_worker)
{
	int ret;

	/* error codes are negative */
	ret = -pthread_create(&diag_worker->thread, &diag_worker->attr,
		diag_func, diag_worker);

	if (ret)
		LOGGING_ERR("Cannot start diagnostic worker %s: %s",
			diag_worker->name, strerror(ret));
	return ret;
}

void stop_diag_workers(void)
{
	struct diag_worker *dw;

	STAILQ_FOREACH(dw, &diag_workers, entries) {
		STAILQ_REMOVE(&diag_workers, dw, diag_worker, entries);
		dw->is_running = false;
	}
}


