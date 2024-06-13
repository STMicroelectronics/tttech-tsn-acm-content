#ifndef ARGS_PARSER_H_
#define ARGS_PARSER_H_


#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

struct diagnostics_param {
	bool enabled;
	unsigned int offset;
	unsigned int mult;
	unsigned int count;
};

struct argv_param {
	int dump_counter; /* -c option */
	unsigned int interval; /* -i option */
	unsigned short int port; /* -p option */
	char host[64]; /* optional argument */
	bool cycles;
	bool missed;
	unsigned int reconnect; /* timeout for reconnect retry in ms */
	unsigned int diag_offs;
	struct diagnostics_param diagnostics;
};

extern void parse_args(int argc, char *argv[], struct argv_param *arg_struct);

#endif /* ARGS_PARSER_H_ */
