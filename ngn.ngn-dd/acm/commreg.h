/* SPDX-License-Identifier: GPL-2.0
 *
 * TTTech ACM Linux driver
 * Copyright(c) 2019 TTTech Industrial Automation AG.
 *
 * Contact Information:
 * support@tttech-industrial.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */
/**
 * @file commreg.h
 * @brief ACM Driver Common Registers
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_COMMREG_H_
#define ACM_COMMREG_H_

/**
 * @addtogroup hwacccommreg
 * @{
 */

#include <linux/kernel.h>
struct acm;
struct commreg;

void commreg_reset(struct commreg *commreg);

const char *commreg_read_version(struct commreg *commreg);
const char *commreg_read_revision(struct commreg *commreg);
u32 commreg_read_device_id(struct commreg *commreg);
bool commreg_read_extended_status(struct commreg *commreg);
bool commreg_read_testmodule_enable(struct commreg *commreg);
bool commreg_read_cfg_read_back(struct commreg *commreg);
bool commreg_read_individual_recovery(struct commreg *commreg);
bool commreg_read_rx_redundancy(struct commreg *commreg);
u32 commreg_read_time_freq(struct commreg *commreg);
unsigned int commreg_read_msgbuf_memsize(struct commreg *commreg);
unsigned int commreg_read_msgbuf_count(struct commreg *commreg);
unsigned int commreg_read_msgbuf_datawidth(struct commreg *commreg);
u32 commreg_read_configuration_id(struct commreg *commreg);
void commreg_write_configuration_id(struct commreg *commreg, u32 id);

int __must_check commreg_init(struct acm *acm);
void commreg_exit(struct acm *acm);

/**@} hwacccommreg*/

#endif /* ACM_COMMREG_H_ */
