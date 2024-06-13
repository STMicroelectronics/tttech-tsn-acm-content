/**
 * @file logging.c
 *
 * Logging
 *
 * These functions are linked in dynamically depending on the configuration.
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include "logging.h"

static int loglevel = LOGLEVEL_DEFAULT;

const char* logprefix[] = {
	[LOGLEVEL_ERR] = "[ERROR]",
	[LOGLEVEL_WARN] = "[WARNING]",
	[LOGLEVEL_INFO] = "[INFO]",
	[LOGLEVEL_DEBUG] = "[DEBUG]"
};

static void logging_stderr(int level, const char *format, va_list args)
{
	fprintf(stderr, "%s ", logprefix[level]);
	vfprintf(stderr, format, args);
	fprintf( stderr, "\n");
}

static void logging_syslog(int level, const char *format, va_list args)
{
	int prio;
	char fmt[32 + strlen(format)];

	switch (level) {
	case LOGLEVEL_ERR:
		prio = LOG_MAKEPRI(LOG_USER, LOG_ERR);
		break;
	case LOGLEVEL_WARN:
		prio = LOG_MAKEPRI(LOG_USER, LOG_WARNING);
		break;
	case LOGLEVEL_INFO:
		prio = LOG_MAKEPRI(LOG_USER, LOG_INFO);
		break;
	case LOGLEVEL_DEBUG:
		prio = LOG_MAKEPRI(LOG_USER, LOG_DEBUG);
		break;
	default:
		return;
	}

	snprintf(fmt, sizeof(fmt), "%s %s", logprefix[level], format);
	vsyslog(prio, fmt, args);
}

static void (*logger)(int level, const char *format, va_list args)
	= logging_stderr;

void logging(int level, const char *format, ...)
{
	va_list args;

	if (level > loglevel)
		return;

	va_start(args, format);
	logger(level, format, args);
	va_end(args);
}

void set_logger(enum logger l)
{
	switch (l) {
	case LOGGER_STDERR:
		logger = logging_stderr;
		break;
	case LOGGER_SYSLOG:
		logger = logging_syslog;
		break;
	}
}


void set_loglevel(int level)
{
	if (LOGLEVEL_ERR <= level && level <= LOGLEVEL_DEBUG)
		loglevel = level;
}
