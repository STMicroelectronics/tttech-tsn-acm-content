/**
 * @file parser.c
 *
 * Arguments Parser
 *
 * These functions are linked in dynamically depending on the configuration.
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#ifndef PARSER_H_
#define PARSER_H_

#include <stdint.h>
#include <stdbool.h>
#include <sched.h>

struct argv_param {
	char		*version;
	bool 		daemon;			/* -d option */
	uint32_t	dump_counter;		/* -c option */
	char 		*lib_name;		/* -l option */
	char 		*conf_name;		/* -f option */
	int 		verbosity;		/* -v option */
	uint32_t	hw_cycle_us;		/* HW cycle time in us */
	cpu_set_t	cpuset;			/* cpu affinity mask */
	bool		distribute;		/* even worker thread distribution */
	int		rxoffset;		/* RX packet counter offset */
#ifdef FTRACE_SUPPORT_ENABLED
	uint32_t	tracelimit;		/* trace limit in us */
	uint32_t	lat_break;		/* latency limit in ns */
	int		break_on_loss;
	int		break_on_double;
	int		break_on_invalid;
	int		break_on_missed;
#endif
	uint32_t	portno;			/* port for monitoring */
	bool 		qa;			/* -q option */
	char		host[64];
	short 		port;
};

void parse_args(int argc, char *argv[], struct argv_param *param);
void free_args(struct argv_param *param);
void log_args(int loglvl, char *name, struct argv_param *args);
int cpu_set_to_int(const cpu_set_t *set);

#endif /* PARSER_H_ */
