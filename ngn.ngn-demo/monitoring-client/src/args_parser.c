#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <libgen.h>

#include "args_parser.h"

static const struct option arg_options[] = {
	{ "counter",		required_argument, 	NULL, 'c' },
	{ "port",		required_argument, 	NULL, 'p' },
	{ "interval",		required_argument, 	NULL, 'i' },
	{ "help", 		no_argument, 		NULL, 'h' },
	{ "cycles",		no_argument, 		NULL, 0 },
	{ "missed",		no_argument, 		NULL, 0 },
	{ "reconnect",		required_argument, 	NULL, 0 },
	{ "diagnostics",	required_argument, 	NULL, 0 },
	{ NULL, 		no_argument, 		NULL, 0 }
};


static void print_usage(char *name)
{
	printf("\nUsage: %s [options] [hostname/ip address]\n", name);
	printf("\tOptions:\n");
	printf("\t-c / --counter <times> do \"times\" dumps (default 0 = run forever).\n");
	printf("\t-i / --interval <millisecs> do dumps each \"millisec\" milliseconds (default: 1000).\n");
	printf("\t-p / --port <portno> connect at port \"portno\" (default 6161).\n");
	printf("\t     --cycles additionally dump cycle data\n");
	printf("\t     --missed count missed intervals\n");
	printf("\t     --reconnect <millisecs> reconnect automatically after <millisecs>, 0 (default) means no reconnect\n");
	printf("\t     --diagnostics <offset>[,<mult>[,<count>]] display diagnostic data read at\n");
	printf("\t                                              <offset> with <mult> multiple of interval <count times>\n");
	printf("\t-[h,?] | --help display the version and this help and exit\n");
	printf("\n");
	printf("Monitoring values:\n");
	printf("\t      RX: Nr. of correct evaluated data\n");
	printf("\t      TX: Nr. of correct updated data\n");
	printf("\t  losses: Nr. of frames not received (rx-timestamp was not updated)\n");
	printf("\t  double: (only if losses is not increased) Nr. of reads with same buffer content\n");
	printf("\t invalid: (only if losses is not increased) Nr. of reads with incorrect buffer content\n");
	printf("\t missed: count interval overflows\n");
	printf("\t      TS: Timestamp when message-buffer was updated [min, mean, max] in nanoseconds\n");
	printf("ACM-module specific values:\n");
	printf("\t  DroppedFramesCounterPrevCycle: Nr. of dropped frames due to policing errors, during previous gate cycle\n");
	printf("\t              TxFramesPrevCycle: Nr. of MAC-level correct frames during previous gating cycle\n");
	printf("\t              RxFramesPrevCycle: Nr. of correctly received frames during previous cycle\n");
	fflush(stdout);
}

static void print_call_args(char *name, struct argv_param *param)
{
	printf( "\n%s is called with following parameters:\n", basename(name));

	if (param->diagnostics.enabled) {
		printf("Reading diagnostic data at %uus ACM cycle offset, each ",
			param->diagnostics.offset);
		switch (param->diagnostics.mult) {
		case 1:
			printf("cycle ");
			break;
		case 2:
			printf("%und cycle ", param->diagnostics.mult);
			break;
		case 3:
			printf("%urd cycle ", param->diagnostics.mult);
			break;
		default:
			printf("%uth cycle ", param->diagnostics.mult);
			break;
		}
		printf("for %u time%s\n", param->diagnostics.count,
			param->diagnostics.count > 1 ? "s" : "");

		return;
	}

	if (param->dump_counter == 1) {
		printf("Stopping after one dump");
	} else {
		if (param->dump_counter == 0) {
			printf("Running forever, dumping");
		} else {
			printf("Stopping after %d dumps", param->dump_counter);
		}
		printf(" each %u msec,", param->interval);
	}
	printf(" trying to connect %s at port %d", param->host, param->port);
	if (param->missed)
		printf(" requesting missed frame count");

	if (param->reconnect)
		printf(" trying reconnect each %ums", param->reconnect);

	printf("\n");
}

const struct argv_param default_args = {
	.dump_counter = 0,	/* run forever */
	.interval = 1000,	/* dump every second */
	.port = 6161,		/* conect to 6161 ... */
	.host = "127.0.0.1",	/* ... on localhost */
	.cycles = false,	/* do not dump cycle data */
	.missed = false,	/* do not dump missed frame counter */
	.reconnect = 0,		/* do not reconnect automatically */
	.diagnostics = { false, 0, 1, 1 }
};

void parse_args(int argc, char *argv[], struct argv_param *args) {
	int opt = 0;
	int index = 0;
	const char *opt_str = "c:p:i:h?";

	/* set defaults */
	*args = default_args;

	opt = getopt_long( argc, argv, opt_str, arg_options, &index );
	while( opt != -1 ) {
		switch (opt) {
		case 'c':
			args->dump_counter = atoi(optarg);
			break;
		case 'p':
			args->port = atoi(optarg);
			break;
		case 'i':
			args->interval = atoi(optarg);
			break;
		case 'h': /* fall-through is intentional */
		case '?':
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		case 0:		/* long option without a short arg */
			if (strcmp( "cycles", arg_options[index].name ) == 0) {
				args->cycles = true;
			}
			if (strcmp( "port", arg_options[index].name ) == 0 ){
				args->port = atoi(optarg);
			}
			if (strcmp( "counter", arg_options[index].name ) == 0) {
				args->dump_counter = atoi(optarg);
			}
			if (strcmp( "interval", arg_options[index].name ) == 0) {
				args->interval = atoi(optarg);
			}
			if (strcmp( "missed", arg_options[index].name ) == 0) {
				args->missed = true;
			}
			if (strcmp("reconnect", arg_options[index].name) == 0) {
				args->reconnect = atoi(optarg);
			}
			if (strcmp("diagnostics", arg_options[index].name) == 0) {
				char *d = strdup(optarg);
				char *p;

				args->diagnostics.enabled = true;

				p = strtok(d, ",");
				args->diagnostics.offset = p ? atoi(p) : 0;
				p = strtok(NULL, ",");
				args->diagnostics.mult = p ? atoi(p) : 1;
				p = strtok(NULL, ",");
				args->diagnostics.count = p ? atoi(p) : 1;
				free(d);
			}
			if (strcmp( "help", arg_options[index].name ) == 0) {
				print_usage(argv[0]);
				exit(EXIT_SUCCESS);
			}
			break;
		default:
			/* You won't actually get here. */
			print_usage(argv[0]);
			exit(EXIT_FAILURE);
			break;
		}

		opt = getopt_long( argc, argv, opt_str, arg_options, &index );
	}

	if (optind == argc - 1)
		strncpy(args->host, argv[optind], sizeof(args->host));
	print_call_args(argv[0], args);
}
