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

#ifndef TRACING_H_
#define TRACING_H_

#define TRACELAYER_0 0  // no traces
#define TRACELAYER_1 1
#define TRACELAYER_2 2
#define TRACELAYER_3 3

#define TRACELAYER_DEFAULT	TRACELAYER_3

#ifndef TRACELAYER
#define TRACELAYER TRACELAYER_DEFAULT
#endif

enum tracer {
	TRACER_STDOUT
};

#ifndef stringify
#define stringify(s) _stringify(s)
#define _stringify(s) #s
#endif

#define TRACE(_tracelayer,_format, ...)	\
	trace(_tracelayer, _format,  ## __VA_ARGS__)

#define TRACE1_MSG(_format, ...)     TRACE(TRACELAYER_1, " -%s: " _format, __func__, ##__VA_ARGS__)
#define TRACE2_MSG(_format, ...)     TRACE(TRACELAYER_2, " -%s: " _format, __func__, ##__VA_ARGS__)
#define TRACE3_MSG(_format, ...)     TRACE(TRACELAYER_3, " -%s: " _format, __func__, ##__VA_ARGS__)

#define TRACE1_ENTER()	TRACE(TRACELAYER_1, "%s called." , __func__)
#define TRACE2_ENTER()	TRACE(TRACELAYER_2, "%s called." , __func__)
#define TRACE3_ENTER()	TRACE(TRACELAYER_3, "%s called." , __func__)

#define TRACE1_EXIT()	TRACE(TRACELAYER_1, "%s exit." , __func__)
#define TRACE2_EXIT()	TRACE(TRACELAYER_2, "%s exit." , __func__)
#define TRACE3_EXIT()	TRACE(TRACELAYER_3, "%s exit." , __func__)

void trace(int layer, const char *format, ...);
void set_tracelayer(int layer);
void set_tracer(enum tracer t);

#endif /* TRACER_H_ */
