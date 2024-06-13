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

#ifndef LIBC_HELPER_H_
#define LIBC_HELPER_H_

#include <fcntl.h>
#include <sys/socket.h>
#include <linux/sockios.h>

#define _to_string(str) #str
#define to_string(str) _to_string(str)

#define LIBC_MOCK(func) libc_register(_to_string(func))
#define LIBC_UNMOCK(func) libc_unregister(_to_string(func))

void libc_register(const char *funcname);
void libc_unregister(const char *funcname);

struct functype {
        const char *name;
        int idx;
        void *func[2];
};

#define FUNC_LIBC 0
#define FUNC_MOCK 1

enum libc_func_id {
    LIBC_OPEN,
    LIBC_CLOSE,
    LIBC_PREAD,
    LIBC_PWRITE,
    LIBC_READ,
    LIBC_SOCKET,
    LIBC_IOCTL,
    LIBC_STRTOULL,
    LIBC___ERRNO_LOCATION,
    LIBC_SNPRINTF,
    LIBC_VSNPRINTF,
    LIBC_VASPRINTF,
    LIBC_ASPRINTF,
    LIBC_FPRINTF,
    LIBC_VFPRINTF,
    LIBC_FFLUSH,
    LIBC_NUM_FUNCTIONS // must be last entry!
};

extern struct functype libc_funcs[LIBC_NUM_FUNCTIONS];

/* list of all supported libc function types */
typedef int (*LIBC_OPEN_TYPE)(const char *filename, int flags, ...);
typedef int (*LIBC_CLOSE_TYPE)(int fd);
typedef ssize_t (*LIBC_PREAD_TYPE)(int fd, void *buf, size_t count, off_t offset);
typedef ssize_t (*LIBC_PWRITE_TYPE)(int fd, const void *buf, size_t count, off_t offset);
typedef ssize_t (*LIBC_READ_TYPE)(int fd, void *buf, size_t nbytes);
typedef int (*LIBC_SOCKET_TYPE)(int domain, int type, int protocol);
typedef int (*LIBC_IOCTL_TYPE)(int fd, unsigned long int request, ...);
typedef unsigned long long int (*LIBC_STRTOULL_TYPE)(const char *nptr, char **endptr, int base);
typedef int *(*LIBC___ERRNO_LOCATION_TYPE)(void);
typedef int (*LIBC_SNPRINTF_TYPE)(char* s, size_t maxlen, const char* format, ...);
typedef int (*LIBC_VSNPRINTF_TYPE)(char *s, size_t maxlen, const char *format, va_list arg);
typedef int (*LIBC_VASPRINTF_TYPE)(char **ptr, const char *f, va_list arg);
typedef int (*LIBC_ASPRINTF_TYPE)(char **ptr, const char *fmt, ...);
typedef int (*LIBC_FPRINTF_TYPE)(FILE *s, const char *format);
typedef int (*LIBC_VFPRINTF_TYPE)(FILE *s, const char *format, va_list arg);
typedef int (*LIBC_FFLUSH_TYPE)(FILE *s);

/* helper to access original libc function within tests using function id */
#define _FUNC(funcid, id) ((funcid ## _TYPE)libc_funcs[funcid].func[id])
#define FUNC_WRAPPER(funcid) _FUNC(funcid, libc_funcs[funcid].idx)
#define FUNC(funcid) _FUNC(funcid, FUNC_LIBC)

#endif /* LIBC_HELPER_H_ */
