/**
 * @file tracer.h
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
#ifndef TRACER_H_
#define TRACER_H_

#ifdef FTRACE_SUPPORT_ENABLED

void setup_tracer(/*unsigned int tracelimit, char *tracetype*/);
void tracing(int on);
void tracemark(char *fmt, ...) __attribute__((format(printf, 1, 2)));

#else

static inline void setup_tracer(/*unsigned int tracelimit, char *tracetype*/)
{
}

static inline void tracing(int on)
{
}

static inline __attribute__((format(printf, 1, 2)))
void tracemark(char *fmt, ...)
{
}


#endif /* FTRACE_SUPPORT_ENABLED */

extern unsigned int tracelimit;

#endif /* TRACER_H_ */
