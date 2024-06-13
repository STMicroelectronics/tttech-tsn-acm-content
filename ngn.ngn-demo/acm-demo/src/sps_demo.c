/**
 * @file sps_demo.c
 *
 * Wrapper for B&R proprietary X2X handler
 *
 * These functions are linked in dynamically depending on the configuration.
 *
 * @copyright (C) 2019 TTTech. All rights reserved. Confidential proprietary.
 *            Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech.com>
 *
 */

#include <stdint.h>
#include <stddef.h>

#ifdef SPS_DEMO
#include <stdlib.h>
#include <errno.h>
#include <x2xhandler.h>
#include <x2xconfig.h>

#include "sps_demo.h"
#include "logging.h"


/* Basic configuration for X20DO322 module */
static const X2XHANDLER_MODULE_DECRIPTOR X20DOF322 = {
		.moduleId = 0xc0ea,
		.numSync = 5,
		.channelsSync = {
			{0, X2XCONFIG_CHANTYPE_STTIN, 1, 0},
			{0, X2XCONFIG_CHANTYPE_FIXOUT,1, 0},
			{1, X2XCONFIG_CHANTYPE_FIXOUT,1, 0},
			{1, X2XCONFIG_CHANTYPE_FIXIN, 1, 0},
			{2, X2XCONFIG_CHANTYPE_FIXIN, 1, 0}
	}
};
/* Basic configuration for X20PS9600 module */
static const X2XHANDLER_MODULE_DECRIPTOR X20PS9600 = {
	.moduleId = 0xeb03,
	.numSync = 4,
	.channelsSync = {
			{0, X2XCONFIG_CHANTYPE_STTIN, 1, 0},
			{1, X2XCONFIG_CHANTYPE_FIXIN, 1, 0},
			{2, X2XCONFIG_CHANTYPE_FIXIN, 1, 0},
			{3, X2XCONFIG_CHANTYPE_FIXIN, 1, 0}
	}
};

static struct X2XHANDLER *hx2x;

int sps_demo_init(void)
{
	int ret;

	/* Create handler instance */
	hx2x = x2xhandlerCreate(0);
	if (hx2x == NULL) {
		LOGGING_ERR("sps_demo: Couldn't create X2X handler\n");
		goto out;
	}

	/* Set initial configuration */
	ret = x2xhandlerBusConfig(hx2x, 2000, 10);
	if (ret != 0) {
		LOGGING_ERR("sps_demo: x2xhandlerBusConfig() failed: %d)\n",
			ret);
		goto out_destroy_hx2x;
	}

	/* Start the X2X bus */
	ret = x2xhandlerStart(hx2x);
	if(ret != 0) {
		LOGGING_ERR("sps_demo: Couldn't start X2X handler (%d)\n", ret);
		goto out_destroy_hx2x;
	}

	/* Configuration of the IO modules (X20DOF322 in the first five slots) */
	x2xhandlerSlotConfig(hx2x, 1, &X20PS9600);
	x2xhandlerSlotConfig(hx2x, 2, &X20DOF322);
	x2xhandlerSlotConfig(hx2x, 3, &X20DOF322);
	x2xhandlerSlotConfig(hx2x, 4, &X20DOF322);
	return 0;

out_destroy_hx2x:
	x2xhandlerDestroy(hx2x);
	hx2x = NULL;
out:
	return -ENODEV;
}

void sps_demo_exit(void)
{
	if (!hx2x)
		return;

	/* Stop x2x bus communication */
	x2xhandlerStop(hx2x);

	/* Delete the handler instance */
	x2xhandlerDestroy(hx2x);
	hx2x = NULL;
}

void sps_demo_write_output(uint8_t *data, size_t len)
{
	if (!hx2x)
		return;

	x2xhandlerOutputWriteImage(hx2x, 0, data, len);
	x2xhandlerOutputApplyImage(hx2x);
}
#else

int sps_demo_init(void)
{
	return 0;
}

void sps_demo_exit(void)
{
}

void sps_demo_write_output(uint8_t *data, size_t len)
{
}


#endif	/* SPS_DEMO */
