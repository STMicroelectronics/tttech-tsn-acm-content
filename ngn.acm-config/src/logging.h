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

#ifndef LOGGING_H_
#define LOGGING_H_

#define LOGLEVEL_DEBUG		3
#define LOGLEVEL_INFO		2
#define LOGLEVEL_WARN		1
#define LOGLEVEL_ERR		0

#define LOGLEVEL_DEFAULT	LOGLEVEL_DEBUG

#ifndef LOGLEVEL
#define LOGLEVEL LOGLEVEL_DEFAULT
#endif

enum logger {
	LOGGER_STDERR,
	LOGGER_SYSLOG
};

#ifndef stringify
#define stringify(s) _stringify(s)
#define _stringify(s) #s
#endif

#define LOG(_loglevel,_format, ...)	\
	logging(_loglevel, _format,  ## __VA_ARGS__)

#define LOGGING_DEBUG(_format, ...)	LOG(LOGLEVEL_DEBUG, _format, ## __VA_ARGS__)
#define LOGGING_INFO(_format, ...)	LOG(LOGLEVEL_INFO, _format, ## __VA_ARGS__)
#define LOGWARN(_format, ...)	LOG(LOGLEVEL_WARN, _format, ## __VA_ARGS__)
#define LOGERR(_format, ...)	LOG(LOGLEVEL_ERR, _format, ## __VA_ARGS__)

void logging(int loglevel, const char *format, ...);
int set_loglevel(int loglevel);
int set_logger(enum logger logger);

#endif /* LOGGING_H_ */
