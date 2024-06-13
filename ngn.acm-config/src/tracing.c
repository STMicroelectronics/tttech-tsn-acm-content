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
#include "tracing.h"

static int tracelayer = TRACELAYER;

const char* traceprefix[] = {
    [TRACELAYER_1] = "[Ext. CALL] ",
    [TRACELAYER_2] = "[T1]     ",
    [TRACELAYER_3] = "[T2]         ",
};

static void trace_stdout(int layer, const char *format, va_list args) {
    fprintf(stdout, "%s", traceprefix[layer]);
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    fflush(stdout);
}

static void (*tracer_print)(int layer, const char *format, va_list args)
	= trace_stdout;

void trace(int layer, const char *format, ...) {
    va_list args;

    if (layer > tracelayer)
        return;

    va_start(args, format);
    tracer_print(layer, format, args);
    va_end(args);
}

void set_tracer(enum tracer t) {
    switch (t) {
        case TRACER_STDOUT:
            tracer_print = trace_stdout;
            break;
    }
}

void set_tracelayer(int layer) {
    if (TRACELAYER_0 <= layer && layer <= TRACELAYER_3)
        tracelayer = layer;
}
