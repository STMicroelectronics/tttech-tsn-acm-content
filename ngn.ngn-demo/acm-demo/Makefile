#******************************************************************************
#  Copyright (c) 2019 TTTech. All rights reserved. Confidential proprietary
#  Schoenbrunnerstrasse 7, A-1040 Wien, Austria. office@tttech.com
# 
#  Name
#    ACM demo and test application
# 
#  Purpose
#    Testing ACM data transfer
# 
#******************************************************************************
MODULE = acm-demo

# handle feature flags for demo
ifneq ($(SPS_DEMO),)
CFLAGS += -DSPS_DEMO -D_X2XLINK_DEMO
LDFLAGS += -lx2xlink
endif

# default flags
CFLAGS += -Wall
CFLAGS += -DFTRACE_SUPPORT_ENABLED
CPPFLAGS +=
LDFLAGS += -lm -lrt -lpthread -ldl
# to be able to resolve all symbols using dlsym()
LDFLAGS += -Wl,--export-dynamic

# add any static library here (must be in library search path)
STATICLIBS +=

# use git version
GIT-VERSION-GEN = ../GIT-VERSION-GEN
GIT-VERSION-PATTERN = "[0-9]*.[0-9]*.[0-9]*"

ACM_DEMO_VERSION = $(GIT_VERSION)
export ACM_DEMO_VERSION
DOXYFILE = doc/acm-demo.doxyfile

# the magic stuff is in here ..
include ../rules.mk
