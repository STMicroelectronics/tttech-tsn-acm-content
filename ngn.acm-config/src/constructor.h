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
 * @file constructor.h
 *
 * startup/initialization of acm lib
 *
 */
#ifndef CONSTRUCTOR_H_
#define CONSTRUCTOR_H_

/**
 * @brief For test purposes static functions are declared as non static
 * @{
 */
#ifndef TEST
#define CTOR __attribute__((constructor)) static
#else
#define CTOR
#endif

/**
 * @brief configure log level and trace level
 *
 * The function reads a values with names "LOGLEVEL" and "TRACELEVEL" from the configuration file
 * and sets log level and trace levels with these values. If the keywords "LOGLEVEL" and/or
 * "TRACELEVEL" not found in the configuration file, then log level and trace level stay at default
 * value.
 * The function prints out information if it changed log level/trace level or not.
 *
 */
CTOR void con(void);


#endif /* CONSTRUCTOR_H_ */
