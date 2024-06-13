// SPDX-License-Identifier: GPL-2.0
/*
 * TTTech ACM Linux driver
 * Copyright(c) 2019 TTTech Industrial Automation AG.
 *
 * Contact Information:
 * support@tttech-industrial.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */
/**
 * @file commreg.c
 * @brief ACM Driver Common Registers
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup hwacccommreg ACM IP Common Registers
 * @brief Provides access to the common registers
 *
 * @{
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/sizes.h>

#include "commreg.h"
#include "acm-module.h"
#include "acmbitops.h"

/**
 * @brief default number of message buffers
 */
#define ACM_COMMREG_MSGBUF_COUNT_DEFAULT	32

/**
 * @brief default message buffers data width
 */
#define ACM_COMMREG_MSGBUF_DATAWIDTH_DEFAULT	4


/**
 * @name Common Register Offsets
 * @brief Register offsets and definitions of the common register block
 */
/**@{*/
#define ACM_COMMREG_DEVICE_ID		0x00000000
#define ACM_COMMREG_VERSION_ID		0x00000004
#define ACM_COMMREG_REVISION_ID		0x00000008

#define ACM_COMMREG_GENERIC_0		0x00000100
#define GENERIC_0_EXT_STATUS		BIT(0)
#define GENERIC_0_TEST_EN		BIT(1)
#define GENERIC_0_CFG_READ_BACK		BIT(2)
#define GENERIC_0_INDIV_RECOV_EN	BIT(3)
#define GENERIC_0_REDUND_RX_EN		BIT(4)
#define GENERIC_0_MSG_BUFF_NO_L		5
#define GENERIC_0_MSG_BUFF_NO_H		11
#define GENERIC_0_MSG_BUFF_NO	\
	GENMASK(GENERIC_0_MSG_BUFF_NO_H, GENERIC_0_MSG_BUFF_NO_L)
#define GENERIC_0_TIME_FREQ_L		16
#define GENERIC_0_TIME_FREQ_H		31
#define GENERIC_0_TIME_FREQ	\
	GENMASK(GENERIC_0_TIME_FREQ_H, GENERIC_0_TIME_FREQ_L)

#define ACM_COMMREG_GENERIC_1		0x00000104
#define GENERIC_1_MSGBUF_ADDR_WIDTH	GENMASK(15, 0)

#define ACM_COMMREG_RST_REQ		0x00000200
#define RST_REQ_SW_RST_REQ		BIT(0)
#define RST_REQ_RST_REQ			BIT(1)

#define ACM_COMMREG_CONFIGURATION_ID	0x00000300
/**@}*/

/**
 * @struct commreg
 * @brief Common Register Block
 *
 * @var commreg::base
 * @brief base address
 *
 * @var commreg::size;
 * @brief module size
 *
 * @var commreg::acm
 * @brief associated ACM instance
 *
 * @var commreg::version[32]
 * @brief ACM IP version
 *
 * @var commreg::revision[128]
 * @brief ACM IP revision
 *
 * @var commreg::device_id
 * @brief ACM IP device ID
 *
 * @var commreg::extended_status
 * @brief ACM IP extended status flag
 *
 * @var commreg::testmodule_enable
 * @brief ACM IP testmodule enable flag
 *
 * @var commreg::time_freq
 * @brief ACM IP scheduler clock frequency
 *
 * @var commreg::msgbuf_memsize
 * @brief ACM IP message buffer size
 *
 * @var commreg::msgbuf_count
 * @brief ACM IP number of message buffers
 *
 * @var commreg::msgbuf_datawidth
 * @brief ACM IP message buff data width
 *
 * @var commreg::cfg_read_back
 * @brief ACM IP config data readback flag
 *
 * @var commreg::individual_recovery
 * @brief ACM IP Individual Recovery Enable
 *
 * @var commreg::rx_redundancy
 * @brief ACM IP Redundancy RX Enable
 */
struct commreg {
	void __iomem	*base;
	resource_size_t	size;

	struct acm	*acm;
	char		version[32];
	char		revision[128];
	u32		device_id;
	bool		extended_status;
	bool		testmodule_enable;
	u32		time_freq;
	unsigned int	msgbuf_memsize;
	unsigned int	msgbuf_count;
	unsigned int	msgbuf_datawidth;
	bool		cfg_read_back;
	bool		individual_recovery;

	bool		rx_redundancy;
};

/**
 * @brief trigger ACM IP reset
 */
void commreg_reset(struct commreg *commreg)
{
	writel(RST_REQ_SW_RST_REQ, commreg->base + ACM_COMMREG_RST_REQ);
}

/**
 * @brief read VERSION_ID register
 */
static u32 commreg_read_version_id(struct commreg *commreg)
{
	return readl(commreg->base + ACM_COMMREG_VERSION_ID);
}

/**
 * @brief read REVISION_ID register
 */
static u32 commreg_read_revision_id(struct commreg *commreg)
{
	return readl(commreg->base + ACM_COMMREG_REVISION_ID);
}

/**
 * @brief read GENERIC_0 register
 */
static u32 commreg_read_generic_0(struct commreg *commreg)
{
	return readl(commreg->base + ACM_COMMREG_GENERIC_0);
}

/**
 * @brief read GENERIC_1 register
 */
static u32 commreg_read_generic_1(struct commreg *commreg)
{
	return readl(commreg->base + ACM_COMMREG_GENERIC_1);
}

/**
 * @brief read cached version of ACM IP
 */
const char *commreg_read_version(struct commreg *commreg)
{
	if (commreg)
		return commreg->version;

	return "unkown";
}

/**
 * @brief read cached revision of ACM IP
 */
const char *commreg_read_revision(struct commreg *commreg)
{
	if (commreg)
		return commreg->revision;

	return "unkown";
}

/**
 * @brief read cached device ID of ACM IP
 */
u32 commreg_read_device_id(struct commreg *commreg)
{
	if (commreg)
		return commreg->device_id;

	return 0;
}

/**
 * @brief read cached extended status enable flag of ACM IP
 */
bool commreg_read_extended_status(struct commreg *commreg)
{
	if (commreg)
		return commreg->extended_status;

	return false;
}

/**
 * @brief read cached testmodule enable flag of ACM IP
 */
bool commreg_read_testmodule_enable(struct commreg *commreg)
{
	if (commreg)
		return commreg->testmodule_enable;

	return false;
}
/**
 * @brief read cached configuration read back flag of ACM IP
 */
bool commreg_read_cfg_read_back(struct commreg *commreg)
{
	if (commreg)
		return commreg->cfg_read_back;

	return true;
}

/**
 * @brief read cached Individual Recovery Enable of ACM IP
 */
bool commreg_read_individual_recovery(struct commreg *commreg)
{
	if (commreg)
		return commreg->individual_recovery;

	return false;
}

/**
 * @brief read cached Redundancy RX Enable of ACM IP
 */
bool commreg_read_rx_redundancy(struct commreg *commreg)
{
	if (commreg)
		return commreg->rx_redundancy;

	return false;
}

/**
 * @brief read cached scheduler clock frequency of ACM IP
 */
u32 commreg_read_time_freq(struct commreg *commreg)
{
	if (commreg)
		return commreg->time_freq;

	return 100000000L;
}

/**
 * @brief read cached message buffer size of ACM IP
 */
unsigned int commreg_read_msgbuf_memsize(struct commreg *commreg)
{
	if (commreg)
		return commreg->msgbuf_memsize;

	return SZ_16K;
}

/**
 * @brief read cached number of message buffers
 */
unsigned int commreg_read_msgbuf_count(struct commreg *commreg)
{
	if (commreg)
		return commreg->msgbuf_count;

	return ACM_COMMREG_MSGBUF_COUNT_DEFAULT;
}

/**
 * @brief read cached number of message buffer datawidth
 */
unsigned int commreg_read_msgbuf_datawidth(struct commreg *commreg)
{
	if (commreg)
		return commreg->msgbuf_datawidth;

	return ACM_COMMREG_MSGBUF_DATAWIDTH_DEFAULT;
}

/**
 * @brief read configuration id register
 */
u32 commreg_read_configuration_id(struct commreg *commreg)
{
	if (!commreg)
		return 0;

	return readl(commreg->base + ACM_COMMREG_CONFIGURATION_ID);
}

/**
 * @brief write configuration id register
 */
void commreg_write_configuration_id(struct commreg *commreg, u32 id)
{
	if (!commreg)
		return;

	return writel(id, commreg->base + ACM_COMMREG_CONFIGURATION_ID);
}

/**
 * @brief Helper to read ACM IP version
 */
static void read_version(struct commreg *commreg)
{
	u32 id;

	id = commreg_read_version_id(commreg);

	if ((id & GENMASK(7, 0)) == 0xFF) {
		scnprintf(commreg->version, sizeof(commreg->version),
				 "unknown");
		return;
	}

	scnprintf(commreg->version, sizeof(commreg->version), "%lu.%lu.%lu",
		  (id & GENMASK(31, 24)) >> 24,
		  (id & GENMASK(23, 16)) >> 16,
		  (id & GENMASK(15, 8)) >> 8);
}

/**
 * @brief Helper to read ACM IP revision
 */
static void read_revision(struct commreg *commreg)
{
	u32 id;

	id = commreg_read_revision_id(commreg);
	scnprintf(commreg->revision, sizeof(commreg->revision), "%x", id);
}

/**
 * @brief initialize Common Register Block
 */
int __must_check commreg_init(struct acm *acm)
{
	struct commreg *commreg;
	struct resource *res;
	struct platform_device *pdev = acm->pdev;
	struct device *dev = &pdev->dev;
	u32 generic_0, generic_1;

	/* no common register block in ACM_IF_1_0 */
	if (acm->if_id == ACM_IF_1_0) {
		acm->commreg = NULL;
		return 0;
	}

	commreg = devm_kzalloc(dev, sizeof(*commreg), GFP_KERNEL);
	if (!commreg)
		return -ENOMEM;

	commreg->acm = acm;
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"CommonRegister");
	commreg->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(commreg->base))
		return PTR_ERR(commreg->base);
	commreg->size = resource_size(res);

	read_version(commreg);
	read_revision(commreg);
	commreg->device_id = readl(commreg->base + ACM_COMMREG_DEVICE_ID);

	generic_0 = commreg_read_generic_0(commreg);
	commreg->extended_status = !!(generic_0 & GENERIC_0_EXT_STATUS);
	commreg->testmodule_enable = !!(generic_0 & GENERIC_0_TEST_EN);
	commreg->cfg_read_back = !!(generic_0 & GENERIC_0_CFG_READ_BACK);
	commreg->individual_recovery = !!(generic_0 & GENERIC_0_INDIV_RECOV_EN);
	commreg->rx_redundancy = !!(generic_0 & GENERIC_0_REDUND_RX_EN);
	commreg->time_freq = read_bitmask(&generic_0, GENERIC_0_TIME_FREQ)
		* 1000 * 1000;
	commreg->msgbuf_count = read_bitmask(&generic_0, GENERIC_0_MSG_BUFF_NO);
	/* fall back to default value */
	if (commreg->msgbuf_count == 0)
		commreg->msgbuf_count = ACM_COMMREG_MSGBUF_COUNT_DEFAULT;

	generic_1 = commreg_read_generic_1(commreg);
	switch (generic_1 & GENERIC_1_MSGBUF_ADDR_WIDTH) {
	case 16:
		commreg->msgbuf_memsize = SZ_64K;
		break;
	case 12:
		commreg->msgbuf_memsize = SZ_16K;
		break;
	default:
		dev_warn(dev, "Unknown messagebuffer size");
		commreg->msgbuf_memsize = -1;
		break;
	}

	dev_dbg(dev, "Probed Common Register Block %pr -> 0x%p\n", res,
		commreg->base);

	/*
	 * Right now the following to capabilties are fixed.
	 * This will change with future AMC IPs and must then be read
	 * from the respective registers.
	 */

	commreg->msgbuf_datawidth = 4;

	acm->commreg = commreg;
	return 0;
}

/**
 * @brief exit Common Register Block
 */
void commreg_exit(struct acm *acm)
{
	/* nothing to do */
}


/**@} hwacccommreg*/
