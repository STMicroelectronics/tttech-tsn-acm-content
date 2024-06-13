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

#include <fcntl.h> /* O_CREAT */
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <dlfcn.h>
#include "unity.h"
#include "libc_helper.h"
#include "libc.h"

/* list all mockable functions */
struct functype libc_funcs[LIBC_NUM_FUNCTIONS] = {
        [LIBC_OPEN] = { .name = "open", 0, { NULL, libc_open } },
        [LIBC_CLOSE] = { .name = "close", 0, { NULL, libc_close } },
        [LIBC_PREAD] = { .name = "pread", 0, { NULL, libc_pread } },
        [LIBC_PWRITE] = { .name = "pwrite", 0, { NULL, libc_pwrite } },
        [LIBC_READ] = { .name = "read", 0, { NULL, libc_read } },
        [LIBC_SOCKET] = { .name = "socket", 0, { NULL, libc_socket } },
        [LIBC_IOCTL] = { .name = "ioctl", 0, { NULL, libc_ioctl } },
        [LIBC_STRTOULL] = { .name = "strtoull", 0, { NULL, libc_strtoull } },
        [LIBC___ERRNO_LOCATION] = { .name = "__errno_location", 0, { NULL, libc___errno_location } },
        [LIBC_SNPRINTF] = { .name = "snprintf", 0, { NULL, libc_snprintf } },
        [LIBC_VSNPRINTF] = { .name = "vsnprintf", 0, { NULL, libc_vsnprintf } },
        [LIBC_VASPRINTF] = { .name = "vasprintf", 0, { NULL, libc_vasprintf } },
        [LIBC_ASPRINTF] = { .name = "asprintf", 0, { NULL, libc_asprintf } },
        [LIBC_FPRINTF] = { .name = "fprintf", 0, { NULL, libc_fprintf } },
        [LIBC_VFPRINTF] = { .name = "vfprintf", 0, { NULL, libc_vfprintf } },
        [LIBC_FFLUSH] = { .name = "fflush", 0, { NULL, libc_fflush } },
};

static void libc_helper_init(void) {
    int i;
    void *libc;

    dlerror(); /* clear all errors */
    libc = dlopen("libc.so.6", RTLD_LAZY | RTLD_NOLOAD);
    if (!libc) {
        char msg[256];

        // TODO: this is broken if snprintf has not yet been handled
        snprintf(msg, sizeof(msg), "GLIBC dlopen error: %s", dlerror());
        TEST_FAIL_MESSAGE(msg);
        return;
    }

    for (i = 0; i < sizeof(libc_funcs)/sizeof(libc_funcs[0]); ++i) {
        struct functype *f = &libc_funcs[i];

        f->func[FUNC_LIBC] = dlsym(libc, libc_funcs[i].name);
        if (!f->func[FUNC_LIBC]) {
            char msg[256];

            // TODO: this is broken if snprintf has not yet been handled
            snprintf(msg, sizeof(msg), "GLIBC dlsym error for %s: %s", libc_funcs[i].name,
                    dlerror());
            TEST_FAIL_MESSAGE(msg);
        }
        f->idx = FUNC_LIBC;
    }

    dlclose(libc);
}

void suite_setup(void) {
    libc_helper_init();
}

void libc_register(const char *funcname) {
    int i;

    for (i = 0; i < sizeof(libc_funcs)/sizeof(libc_funcs[0]); ++i) {
        if (strcmp(libc_funcs[i].name, funcname))
            continue;

        libc_funcs[i].idx = FUNC_MOCK;
        break;
    }
}

void libc_unregister(const char *funcname) {
    int i;

    for (i = 0; i < sizeof(libc_funcs)/sizeof(libc_funcs[0]); ++i) {
        if (strcmp(libc_funcs[i].name, funcname))
            continue;

        libc_funcs[i].idx = FUNC_LIBC;
        break;
    }
}

int open(const char *filename, int flags, ...) {
    int mode = 0;

    if (flags & O_CREAT) {
        va_list args;

        va_start(args, flags);
        mode = va_arg(args, int);
        va_end(args);
    }

    return FUNC_WRAPPER(LIBC_OPEN)(filename, flags, mode);
}

int close(int fd) {
    return FUNC_WRAPPER(LIBC_CLOSE)(fd);
}

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
    return FUNC_WRAPPER(LIBC_PREAD)(fd, buf, count, offset);
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
    return FUNC_WRAPPER(LIBC_PWRITE)(fd, buf, count, offset);
}

ssize_t read(int fd, void *buf, size_t nbytes)
{
    return FUNC_WRAPPER(LIBC_READ)(fd, buf, nbytes);
}

int socket(int domain, int type, int protocol) {
    return FUNC_WRAPPER(LIBC_SOCKET)(domain, type, protocol);
}

int ioctl(int fd, unsigned long int request, ...) {
    va_list args;
    int ret;

    va_start(args, request);

    switch (request) {
        case SIOCGIFHWADDR:
        {
            struct ifreq *ifr;

            ifr = va_arg(args, typeof(ifr));
            ret = FUNC_WRAPPER(LIBC_IOCTL)(fd, request, ifr);
            break;
        }
        default:
        {
            char msg[256];

            snprintf(msg, sizeof(msg), "ioctl mock not supported for 0x%08lx", request);
            TEST_FAIL_MESSAGE(msg);
            break;
        }

    }
    va_end(args);

    return ret;
}

unsigned long long int strtoull(const char *nptr, char **endptr, int base) {
    return FUNC_WRAPPER(LIBC_STRTOULL)(nptr, endptr, base);
}

/* to control errno access */
int *__errno_location(void) {
    return FUNC_WRAPPER(LIBC___ERRNO_LOCATION)();
}

int snprintf(char* s, size_t maxlen, const char* format, ...) {
    int ret;

    if (libc_funcs[LIBC_SNPRINTF].idx == FUNC_LIBC) {
        va_list args;

        va_start(args, format);
        ret = FUNC(LIBC_VSNPRINTF)(s, maxlen, format, args);
        va_end(args);
    } else {
        ret = _FUNC(LIBC_SNPRINTF, FUNC_MOCK)(s, maxlen, format);
    }
    return ret;
}

int vsnprintf(char *s, size_t maxlen, const char *format, va_list arg) {
    return FUNC_WRAPPER(LIBC_VSNPRINTF)(s, maxlen, format, arg);
}

int vasprintf(char **ptr, const char *f, va_list arg) {
    return FUNC_WRAPPER(LIBC_VASPRINTF)(ptr, f, arg);
}

int asprintf(char **ptr, const char *fmt, ...) {
    int ret;

    if (libc_funcs[LIBC_ASPRINTF].idx == FUNC_LIBC) {
        va_list args;

        va_start(args, fmt);
        ret = FUNC(LIBC_VASPRINTF)(ptr, fmt, args);
        va_end(args);
    } else {
        ret = _FUNC(LIBC_ASPRINTF, FUNC_MOCK)(ptr, fmt);
    }
    return ret;
}

/* TODO: investigate: does not work as expected, since XXX_ioputs(?) is used instead of fprintf */
int fprintf(FILE *s, const char *format, ...) {
    int ret;

    if (libc_funcs[LIBC_FPRINTF].idx == FUNC_LIBC) {
        va_list args;

        va_start(args, format);
        ret = FUNC(LIBC_VFPRINTF)(s, format, args);
        va_end(args);
    } else {
        ret = _FUNC(LIBC_FPRINTF, FUNC_MOCK)(s, format);
    }
    return ret;
}

int vfprintf(FILE *s, const char *format, va_list arg) {
    return FUNC_WRAPPER(LIBC_VFPRINTF)(s, format, arg);
}

int fflush(FILE *s) {
    return FUNC_WRAPPER(LIBC_FFLUSH)(s);
}
