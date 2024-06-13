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
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sched.h>

#include "parser.h"
#include "logging.h"

static const struct option options[] = {
	{ "daemon",		no_argument, 		NULL, 'd' },
	{ "counter",		required_argument, 	NULL, 'c' },
	{ "conf_name",		required_argument, 	NULL, 'f' },
	{ "verbosity",		no_argument, 		NULL, 'v' },
	{ "cycle",		required_argument, 	NULL, 0 },
#ifdef FTRACE_SUPPORT_ENABLED
	{ "tracelimit",		required_argument, 	NULL, 0 },
	{ "break",		required_argument, 	NULL, 'b' },
	{ "break-on-loss",	required_argument, 	NULL, 0 },
	{ "break-on-double",	required_argument, 	NULL, 0 },
	{ "break-on-invalid",	required_argument, 	NULL, 0 },
	{ "break-on-missed",	required_argument, 	NULL, 0 },
#endif
	{ "monitor-port",	required_argument,	NULL, 'p' },
	{ "qa",			no_argument, 		NULL, 'q' },
	{ "host",		required_argument, 	NULL, 0 },
	{ "port",		required_argument, 	NULL, 0 },
	{ "cpu-mask",		required_argument, 	NULL, 0 },
	{ "help",		no_argument, 		NULL, 'h' },
	{ "version",		no_argument, 		NULL, 0 },
	{ "rxoffset",		required_argument, 	NULL, 0 },
	{ "distribute",		no_argument,		NULL, 0 },
	{ NULL, 		no_argument,		NULL, 0 }
};

static void print_usage(char *varName)
{
	printf("\nUsage: %s [options]\n", varName);
	printf("\tOptions:\n");
	printf("\t-d / --daemon                   start program as a daemon process\n");
	printf("\t-c / --counter <frame counter>  shutdown the demo after \"frame counter\" frames. Enabling this flag will\n");
	printf("\t                                automatically disable the -d flag.\n");
	printf("\t-f / --conf_name <name>         Name of the demo configuration file.\n");
	printf("\t-v / --verbosity                verbosity.\n");
	printf("\t     --cycle <us>               HW cycle time in us.\n");
#ifdef FTRACE_SUPPORT_ENABLED
	printf("\t     --tracelimit <us>          send break trace command when worker time > <us>\n");
	printf("\t-b / --break <us>               stop tracer when max. latency exceeds <us>.\n");
	printf("\t     --break-on-loss <count>    stop tracer when <count> losses are reached.\n");
	printf("\t     --break-on-double <count>  stop tracer when <count> doubles are reached.\n");
	printf("\t     --break-on-invalid <count> stop tracer when <count> invalids are reached.\n");
	printf("\t     --break-on-missed <count>  stop tracer when <count> missed are reached.\n");
#endif
	printf("\t-p / --monitor-port             monitor port number (defaults to 6161)\n");
	printf("\t-q / --qa                       Quality assurance. Frames content will be passed via UDP to a Host.\n");
	printf("\t     --host <ip-address>        Host PC IP.\n");
	printf("\t     --port <port>              Host PC UDP-Port.\n");
	printf("\t     --cpu-mask <mask value>    CPU Bitmask for worker threads.\n");
	printf("\t     --distribute               Distribute workers on available CPU cores.\n");
	printf("\t     --rxoffset <packet count>  Offset for RX packet count check (defaults to 1).\n");
	printf("\t-[h,?] | --help                 display the version and this help and exit\n");
	fflush(stdout);
}

int cpu_set_to_int(const cpu_set_t *set)
{
	int i;
	int mask = 0;

	for (i = 0; i < 8 * sizeof(int); ++i) {
		if (CPU_ISSET(i, set))
			mask |= (1 << i);
	}

	return mask;
}

void int_to_cpu_set(int mask, cpu_set_t *set)
{
	int i;
	CPU_ZERO(set);

	for (i = 0; i < 8 * sizeof(int); ++i) {
		if (mask & (1 << i))
			CPU_SET(i, set);
	}
}

void log_args(int loglvl, char *name, struct argv_param *args)
{
	/* ... */
	LOG(loglvl, "%s has been called with the following parameters:", name);
	LOG(loglvl, "daemonize: %s", (args->daemon) ? "true" : "false" );
	if (args->dump_counter == 0) {
		LOG(loglvl, "Run forever");
	} else {
		LOG(loglvl, "Stop after %u frames", args->dump_counter );
	}
	LOG(loglvl, "config file: %s", args->conf_name );
	LOG(loglvl, "verbosity = %d", args->verbosity);
	LOG(loglvl, "HW cycle = %uus", args->hw_cycle_us);
	LOG(loglvl, "CPU mask = %u", cpu_set_to_int(&args->cpuset));
	LOG(loglvl, "distribute = %s", args->distribute ? "enabled" : "disabled");
	LOG(loglvl, "RX packet counter offset = %u", args->rxoffset);
#ifdef FTRACE_SUPPORT_ENABLED
	if (args->tracelimit > 0)
		LOG(loglvl, "tracelimit = %uus", args->tracelimit);
	else
		LOG(loglvl, "tracelimit: disabled");
	if (args->lat_break > 0)
		LOG(loglvl, "break = %uus", args->lat_break / 1000);
	else
		LOG(loglvl, "break: disabled");
#endif
	LOG(loglvl, "Monitor port = %u", args->portno);
	LOG(loglvl, "QA host = %s, Port = %d", args->host, args->port);

}

void parse_args(int argc, char *argv[], struct argv_param *param) {
	int opt = 0;
	int lindex = 0;
#ifdef FTRACE_SUPPORT_ENABLED
	const char *opt_str = "db:c:f:qvhp:?";
#else
	const char *opt_str = "dc:f:qvhp:?";
#endif
	asprintf(&param->version, "%s", stringify(GIT_VERSION));
	param->daemon = false;		/* -d option */
	param->dump_counter = 0;	/* -c option */
	param->conf_name = NULL;	/* -f option */
	param->verbosity = 0;		/* -v option */
	param->hw_cycle_us = 1000;	/* default to 1ms */
	CPU_ZERO(&param->cpuset);
	sched_getaffinity(0, sizeof(param->cpuset), &param->cpuset);
	param->distribute = false;
	param->rxoffset = 1;
#ifdef FTRACE_SUPPORT_ENABLED
	param->tracelimit = 0;		/* 0 means off */
	param->lat_break = 0;		/* 0 means off */
	param->break_on_loss = 0;
	param->break_on_double = 0;
	param->break_on_invalid = 0;
	param->break_on_missed = 0;
#endif
	param->portno = 6161;
	param->qa = true;		/* -q option */
	snprintf(param->host, sizeof(param->host), "%s", "192.168.6.161");
	param->port = 1040;
	/* Process the arguments with getopt_long(), then
	 * populate globalArgs.
	 */
	opt = getopt_long(argc, argv, opt_str, options, &lindex);
	while( opt != -1 ) {
		switch( opt ) {
		case 'd':
			param->daemon = true; /* true */
			break;
		case 'c':
			param->dump_counter = atoi(optarg);
			break;
#ifdef FTRACE_SUPPORT_ENABLED
		case 'b':
			param->lat_break = atoi(optarg) * 1000;
			break;
#endif
		case 'f':
			param->conf_name = optarg;
			break;
		case 'v':
			if (param->verbosity < LOGLEVEL_DEBUG)
				param->verbosity++;
			break;
		case 'p':
			param->portno = atoi(optarg);
			break;
		case 'q':
			param->qa = true; /* -q option */
			break;
		case 'h': /* fall-through is intentional */
		case '?':
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
			break;

		case 0: /* long option without a short arg */
			if (strcmp("version", options[lindex].name) == 0) {
				printf("%s\n", param->version);
				exit(EXIT_SUCCESS);
			}
			if (strcmp("cycle", options[lindex].name) == 0) {
				param->hw_cycle_us = atoi(optarg);
			}
#ifdef FTRACE_SUPPORT_ENABLED
			if (strcmp("tracelimit", options[lindex].name) == 0 ) {
				param->tracelimit = atoi(optarg);
			}
			if (strcmp("break", options[lindex].name) == 0 ) {
				param->lat_break = atoi(optarg) * 1000;
			}
			if (strcmp("break-on-loss", options[lindex].name) == 0 ) {
				param->break_on_loss = atoi(optarg);
			}
			if (strcmp("break-on-double", options[lindex].name) == 0 ) {
				param->break_on_double = atoi(optarg);
			}
			if (strcmp("break-on-invalid", options[lindex].name) == 0 ) {
				param->break_on_invalid = atoi(optarg);
			}
			if (strcmp("break-on-missed", options[lindex].name) == 0 ) {
				param->break_on_missed = atoi(optarg);
			}
#endif
			if (strcmp("host", options[lindex].name) == 0) {
				snprintf(param->host, sizeof(param->host),
					optarg);
			}
			if (strcmp("port", options[lindex].name) == 0) {
				param->port = atoi(optarg);
			}
			if (strcmp("monitor-port", options[lindex].name) == 0) {
				param->portno = atoi(optarg);
			}
			if (strcmp("cpu-mask", options[lindex].name) == 0) {
				cpu_set_t set;

				int_to_cpu_set(atoi(optarg), &set);
				CPU_AND(&param->cpuset, &param->cpuset,
					&set);
			}
			if (strcmp("rxoffset", options[lindex].name) == 0) {
				param->rxoffset = atoi(optarg);
			}
			if (strcmp("distribute", options[lindex].name) == 0) {
				param->distribute = true;
			}
			break;

		default:
			/* You won't actually get here. */
			print_usage(argv[0]);
			exit(EXIT_FAILURE);
			break;
		}

		opt = getopt_long( argc, argv, opt_str, options, &lindex );
	}
}

void free_args(struct argv_param *param)
{
	free(param->version);
}

