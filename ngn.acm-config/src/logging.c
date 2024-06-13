/*
 * TTTech ACM Configuration Library (libacmconfig)
 * Copyright(c) 2019 TTTech Industrial Automation AG.
 *
 * ALL RIGHTS RESERVED.
 * Usage of this software, including source code, netlists, documentation,
 * is subject to restrictions and conditions of the applicable license
 * agreement with TTTech Industrial Automation AG or its affiliates.
 *
 * All trademarks used are the property of their respective owners.
 *
 * TTTech Industrial Automation AG and its affiliates do not assume any liability
 * arising out of the application or use of any product described or shown
 * herein. TTTech Industrial Automation AG and its affiliates reserve the right to
 * make changes, at any time, in order to improve reliability, function or
 * design.
 *
 * Contact: https://tttech.com * support@tttech.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include "logging.h"

static int loglevel = LOGLEVEL_DEFAULT;

const char* logprefix[] = {
    [LOGLEVEL_ERR] = "[ERROR]",
    [LOGLEVEL_WARN] = "[WARNING]",
    [LOGLEVEL_INFO] = "[INFO]",
    [LOGLEVEL_DEBUG] = "[DEBUG]"
};

static void logging_stderr(int level, const char *format, va_list args) {
    fprintf(stderr, "%s ", logprefix[level]);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    fflush(stderr);
}

static void logging_syslog(int level, const char *format, va_list args) {
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

    snprintf(fmt, sizeof (fmt), "%s %s", logprefix[level], format);
    vsyslog(prio, fmt, args);
}

static void (*logger)(int level, const char *format, va_list args) = logging_stderr;

void logging(int level, const char *format, ...) {
    va_list args;

    if (level > loglevel)
        return;

    va_start(args, format);
    logger(level, format, args);
    va_end(args);
}

int set_logger(enum logger l) {
    int ret = 0;

    switch (l) {
        case LOGGER_STDERR:
            logger = logging_stderr;
            break;
        case LOGGER_SYSLOG:
            logger = logging_syslog;
            break;
        default:
            ret = -EINVAL;
            break;
    }

    return ret;
}

int set_loglevel(int level) {
    if (level < LOGLEVEL_ERR || LOGLEVEL_DEBUG < level)
        return -EINVAL;

    loglevel = level;
    return 0;
}
