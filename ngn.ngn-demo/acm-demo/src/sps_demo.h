/**
 * @file sps_demo.h
 *
 * Wrapper for B&R proproetary X2X handler
 *
 * These functions are linked in dynamically depending on the configuration.
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */
#ifndef SPS_DEMO_H_
#define SPS_DEMO_H_

#include <stddef.h>
#include <stdint.h>

int sps_demo_init(void);
void sps_demo_exit(void);
void sps_demo_write_output(uint8_t *data, size_t len);

#endif /* SPS_DEMO_H_ */
