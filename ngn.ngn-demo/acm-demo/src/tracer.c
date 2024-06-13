/**
 * @file tracer.c
 *
 * Helpers for kernel function tracer handling
 *
 * These functions are linked in dynamically depending on the configuration.
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#ifdef FTRACE_SUPPORT_ENABLED

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include "logging.h"

static int trace_fd;
static int tracemark_fd;

void setup_tracer(void)
{
	const char *trace_enable = "/sys/kernel/debug/tracing/tracing_on";
	const char *marker = "/sys/kernel/debug/tracing/trace_marker";

	trace_fd = open(trace_enable, O_WRONLY);
	if (trace_fd < 0) {
		LOGGING_WARN("tracer: unable to open tracing_on file %s: %s\n",
			trace_enable, strerror(errno));
		return;
	}

	tracemark_fd = open(marker, O_WRONLY);
	if (tracemark_fd < 0) {
		LOGGING_WARN("tracer: unable to open trace_marker file %s: %s",
			marker, strerror(errno));
		return;
	}
}

void tracing(int on)
{
	if (on)
		write(trace_fd, "1", 1);
	else
		write(trace_fd, "0", 1);
}

#define TRACEBUFSIZ 1024

__attribute__((format(printf, 1, 2))) void tracemark(char *fmt, ...)
{
	va_list ap;
	int len;
	char tracebuf[TRACEBUFSIZ];


	va_start(ap, fmt);
	len = vsnprintf(tracebuf, TRACEBUFSIZ, fmt, ap);
	va_end(ap);

	/* write the tracemark message */
	write(tracemark_fd, tracebuf, len);
}

#endif /* FTRACE_SUPPORT_ENABLED */
