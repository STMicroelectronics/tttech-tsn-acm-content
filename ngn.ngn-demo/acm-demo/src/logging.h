/**
 * @file logging.h
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
#define LOGGING_WARN(_format, ...)	LOG(LOGLEVEL_WARN, _format, ## __VA_ARGS__)
#define LOGGING_ERR(_format, ...)	LOG(LOGLEVEL_ERR, _format, ## __VA_ARGS__)

void logging(int loglevel, const char *format, ...);
void set_loglevel(int loglevel);
void set_logger(enum logger logger);

#endif /* LOGGING_H_ */
