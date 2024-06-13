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

/**
 * @file memory.h
 *
 * memory management functions for acm lib
 *
 */
#ifndef MEMORY_H_
#define MEMORY_H_

#include <stddef.h>
#include <errno.h>

/**
 * @brief allocate memory
 *
 * The function allocates memory of required size and initializes it with zero.
 *
 * @param size amount of memory (in bytes) to be allocated
 *
 * @return The function returns a pointer to the allocated memory. If no memory was allocated,
 * the function returns NULL.
 */
void *acm_zalloc(size_t size);

/**
 * @brief release memory
 *
 * The function releases the memory where mem points to.
 *
 * @param mem pointer to memory to be released
 *
 */
void acm_free(void *mem);

/**
 * @brief duplicate a string
 *
 * The function allocates memory and copies the string of input parameter s to the allocated
 * memory. The duplicated string has to be released with free().
 *
 * @param s string to be duplicated
 *
 * @return The function returns a pointer to the duplicated string. In case of an error
 * the function returns NULL.
 */
char *acm_strdup(const char *s);

#endif /* MEMORY_H_ */
