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

#ifndef LIBC_H_
#define LIBC_H_

#include <stddef.h> /* size_t */
#include <sys/types.h> /* off_t */

int *libc___errno_location(void);

int libc_open(const char *filename, int flags, ...);
ssize_t libc_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t libc_pwrite(int fd, const void *buf, size_t count, off_t offset);
ssize_t libc_read(int fd, void *buf, size_t nbytes);
int libc_close(int fd);
int libc_socket(int domain, int type, int protocol);
int libc_ioctl(int fd, unsigned long int request, ...);
unsigned long long int libc_strtoull(const char *nptr, char **endptr, int base);

/* remark: remove variadic parts for mocking */
int libc_snprintf(char* s, size_t maxlen, const char* format);
int libc_vsnprintf(char *s, size_t maxlen, const char *format, va_list arg);
int libc_vasprintf(char **ptr, const char *f, va_list arg);
int libc_asprintf(char **ptr, const char *fmt);
int libc_fprintf(FILE *s, const char *format);
int libc_vfprintf(FILE *s, const char *format, va_list arg);
int libc_fflush(FILE *s);

#endif /* LIBC_H_ */
