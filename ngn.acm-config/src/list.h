/*
 * TTTech ACM Configuration Library (libacmconfig)
 * Copyright(c) 2019 TTTech Industrial Automation AG.
 *
 * ALL RIGHTS RESERVED.
 * Usage of this software, including source code, nettailqs, documentation,
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

#ifndef LIST_H_
#define LIST_H_

#include <pthread.h>
#include <sys/queue.h>

 /* fixup scan-build issue */
#undef TAILQ_LAST
#define TAILQ_LAST(head, headname) \
    (*(((struct headname *)(void *)((head)->tqh_last))->tqh_last))

#define ACMLIST_HEAD(name, type)          \
struct name {                             \
    pthread_mutex_t lock;                 \
    size_t num;                           \
    TAILQ_HEAD(name##_tailq, type) tailq; \
}

#define ACMLIST_HEAD_INITIALIZER(head)            \
{                                                 \
    .lock = PTHREAD_MUTEX_INITIALIZER,            \
    .num = 0,                                     \
    .tailq = TAILQ_HEAD_INITIALIZER((head).tailq) \
}

#define ACMLIST_ENTRY(reftype, type) \
struct {                             \
    struct reftype *tqh;             \
    TAILQ_ENTRY(type) tqe;           \
}

#define ACMLIST_ENTRY_INITIALIZER \
{                                 \
    .tqh = NULL,                  \
    .tqe = { NULL, NULL }         \
}

#define ACMLIST_ENTRY_INIT(_entry) do { \
    (_entry)->tqh = NULL;               \
    (_entry)->tqe.tqe_next = NULL;      \
    (_entry)->tqe.tqe_prev = NULL;      \
} while(0)

#define ACMLIST_LOCK(head)      pthread_mutex_lock(&((head)->lock))
#define ACMLIST_UNLOCK(head)    pthread_mutex_unlock(&((head)->lock))
#define ACMLIST_COUNT(head)     ((head)->num)
#define ACMLIST_REF(elm, field) ((elm)->field.tqh)

#define _ACMLIST_INIT(head) do {  \
    ACMLIST_COUNT(head) = 0;      \
    TAILQ_INIT(&((head)->tailq)); \
} while (0)

#define ACMLIST_INIT(head) do {                \
    pthread_mutex_init(&((head)->lock), NULL); \
    _ACMLIST_INIT(head);                       \
} while (0)

#define _ACMLIST_INSERT_HEAD(head, elm, field) do {     \
    TAILQ_INSERT_HEAD(&((head)->tailq), elm, field.tqe); \
    ACMLIST_REF(elm, field) = (head);                   \
    ACMLIST_COUNT(head)++;                              \
} while (0)

#define ACMLIST_INSERT_HEAD(head, elm, field) do {  \
    ACMLIST_LOCK(head);                             \
    _ACMLIST_INSERT_HEAD(head, elm, field);         \
    ACMLIST_UNLOCK(head);                           \
} while (0)

#define _ACMLIST_INSERT_TAIL(head, elm, field) do {      \
    TAILQ_INSERT_TAIL(&((head)->tailq), elm, field.tqe); \
    ACMLIST_REF(elm, field) = (head);                    \
    ACMLIST_COUNT(head)++;                               \
} while (0)

#define ACMLIST_INSERT_TAIL(head, elm, field) do {  \
    ACMLIST_LOCK(head);                             \
    _ACMLIST_INSERT_TAIL(head, elm, field);         \
    ACMLIST_UNLOCK(head);                           \
} while (0)

#define _ACMLIST_INSERT_AFTER(head, tailqelm, elm, field) do {        \
    TAILQ_INSERT_AFTER(&((head)->tailq), tailqelm, elm, field.tqe);   \
    ACMLIST_REF(elm, field) = (head);                                 \
    ACMLIST_COUNT(head)++;                                            \
} while (0)

#define ACMLIST_INSERT_AFTER(head, tailqelm, elm, field) do { \
    ACMLIST_LOCK(head);                                       \
    _ACMLIST_INSERT_AFTER(head, tailqelm, elm, field);        \
    ACMLIST_UNLOCK(head);                                     \
} while (0)

#define _ACMLIST_INSERT_BEFORE(head, tailqelm, elm, field) do { \
    TAILQ_INSERT_BEFORE(tailqelm, elm, field.tqe);              \
    ACMLIST_REF(elm, field) = (head);                           \
    ACMLIST_COUNT(head)++;                                      \
} while (0)

#define ACMLIST_INSERT_BEFORE(head, tailqelm, elm, field) do { \
    ACMLIST_LOCK(head);                                        \
    _ACMLIST_INSERT_BEFORE(head, tailqelm, elm, field);        \
    ACMLIST_UNLOCK(head);                                      \
} while (0)

#define _ACMLIST_REMOVE(head, elm, field) do {      \
    TAILQ_REMOVE(&((head)->tailq), elm, field.tqe); \
    ACMLIST_REF(elm, field) = NULL;                 \
    ACMLIST_COUNT(head)--;                          \
} while (0)

#define ACMLIST_REMOVE(head, elm, field) do {  \
    ACMLIST_LOCK(head);                        \
    _ACMLIST_REMOVE(head, elm, field);         \
    ACMLIST_UNLOCK(head);                      \
} while (0)

#define ACMLIST_FOREACH(var, head, field) TAILQ_FOREACH(var, &((head)->tailq), field.tqe)
#define ACMLIST_FOREACH_REVERSE(var, head, headname, field) \
    TAILQ_FOREACH_REVERSE(var, &((head)->tailq), tailq, field.tqe)

#define ACMLIST_EMPTY(head)       TAILQ_EMPTY(&((head)->tailq))
#define ACMLIST_FIRST(head)       TAILQ_FIRST(&((head)->tailq))
#define ACMLIST_NEXT(elm, field)  TAILQ_NEXT(elm, field.tqe)

#define ACMLIST_LAST(head, headname) \
    TAILQ_LAST(&((head)->tailq), headname##_tailq)
#define ACMLIST_PREV(elm, headname, field) \
    TAILQ_PREV(elm, tailq, field.tqe)

#endif /* LIST_H_ */
