/**
 * @file demo.h
 *
 * ACM demo main
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#ifndef DEMO_H_
#define DEMO_H_

#include <stdint.h>
#include <time.h>
#include <sched.h>

#define USEC_PER_SEC	1000000LL
#define NSEC_PER_USEC	1000LL
#define NSEC_PER_MSEC	1000000LL
#define NSEC_PER_SEC	1000000000LL

static inline void tsnorm(struct timespec *ts)
{
	while (ts->tv_nsec >= NSEC_PER_SEC) {
		ts->tv_nsec -= NSEC_PER_SEC;
		ts->tv_sec++;
	}
}

static inline int tsgreater(struct timespec *a, struct timespec *b)
{
	return ((a->tv_sec > b->tv_sec) ||
		(a->tv_sec == b->tv_sec && a->tv_nsec > b->tv_nsec));
}

static inline void tsinc(struct timespec *time, uint32_t increment)
{
	struct timespec next;

	next = *time;
	next.tv_nsec = time->tv_nsec + increment;
	tsnorm(&next);

	*time = next;
}

static inline uint64_t tsdiff_ns(struct timespec *a, struct timespec *b)
{
	return (a->tv_sec - b->tv_sec) * NSEC_PER_SEC +
		(a->tv_nsec - b->tv_nsec);
}

uint32_t get_param_hw_cycle_us(void);
uint32_t get_param_max_frame_counter(void);
#ifdef FTRACE_SUPPORT_ENABLED
uint32_t get_param_tracelimit(void);
uint32_t get_param_lat_break(void);
int get_param_break_on_loss(void);
int get_param_break_on_double(void);
int get_param_break_on_invalid(void);
int get_param_break_on_missed(void);
#else
static inline uint32_t get_param_tracelimit(void)
{
	return 0;
}

static inline uint32_t get_param_lat_break(void)
{
	return 0;
}

static inline int get_param_break_on_loss(void)
{
	return 0;
}

static inline int get_param_break_on_double(void)
{
	return 0;
}

static inline int get_param_break_on_invalid(void)
{
	return 0;
}

static inline int get_param_break_on_missed(void)
{
	return 0;
}

#endif /* FTRACE_SUPPORT_ENABLED */

uint32_t get_param_portno(void);
uint16_t get_param_pc_port(void);
const char *get_param_pc_host(void);
const cpu_set_t *get_param_cpuset(void);
const  bool get_param_distribute(void);
int get_param_rxoffset(void);

void force_stop(void);

#endif /* DEMO_H_ */
