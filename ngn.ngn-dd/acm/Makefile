# SPDX-License-Identifier: GPL-2.0
#
# TTTech ACM Linux driver
# Copyright(c) 2019 TTTech Industrial Automation AG.
#
# Contact Information:
# support@tttech-industrial.com
# TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria

CONFIG_MODULE_SIG = y
CONFIG_MODULE_SIG_ALL = y

DOXYBUILDDIR = build/doc
LATEXDIR = $(DOXYBUILDDIR)/latex
DOCINSTALLDIR = /usr/share/doc/acm

ifeq ($(KDIR),)
KDIR := $(KERNEL_SRC)
endif
ifeq ($(KDIR),)
KDIR := /lib/modules/$(shell uname -r)/build
endif

TTTECH_DRV_PATH = $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../)
ifeq ($(FLX_MODULE_PATH),)
FLX_MODULE_PATH = $(TTTECH_DRV_PATH)/edge-lkm
endif
ACM_DRV_PATH = $(TTTECH_DRV_PATH)/acm

ACM_DRIVER_VERSION = $(shell git -C $(ACM_DRV_PATH) describe HEAD)
export ACM_DRIVER_VERSION

ccflags-y += -I$(ACM_DRV_PATH) -I$(FLX_MODULE_PATH) -DACMDRV_VERSION=$(ACM_DRIVER_VERSION)

acm-objs =		\
	main.o		\
	bypass.o	\
	scheduler.o	\
	redundancy.o	\
	msgbuf.o	\
	sysfs.o		\
	sysfs_control.o	\
	sysfs_config.o	\
	sysfs_status.o	\
	sysfs_error.o	\
	sysfs_diag.o	\
	state.o		\
	config.o	\
	chardev.o	\
	reset.o		\
	commreg.o

obj-m += acm.o

all : modules

modules modules_install clean :
	@echo Building $@ via $(KDIR)
	$(MAKE) -C $(KDIR) M=$(ACM_DRV_PATH) $@

coccicheck :
	@echo Coccichecking via $(KDIR)
	$(MAKE) -C $(KDIR) M=$(ACM_DRV_PATH) MODE=report coccicheck

$(LATEXDIR)/refman.pdf : doxygen
	$(MAKE) -C $(LATEXDIR) > $(abspath $(DOXYBUILDDIR)/pdflatex.log) 2>&1

$(DOXYBUILDDIR)/acm.pdf : $(LATEXDIR)/refman.pdf
	@cp $< $@

doxygen :
	@mkdir -p $(DOXYBUILDDIR)
	@doxygen doc/acm.doxyfile
	@cd $(DOXYBUILDDIR) ; zip -q -r -9 acm.html.zip html

doc : doxygen $(DOXYBUILDDIR)/acm.pdf

doc-clean :
	@rm -rf build

install-doc : $(DOXYBUILDDIR)/acm.pdf
	@install -d $(DESTDIR)$(DOCINSTALLDIR) && \
		install -m 0644 $(wildcard $(DOXYBUILDDIR)/*.zip) \
		$(wildcard $(DOXYBUILDDIR)/*.pdf) \
		$(DESTDIR)$(DOCINSTALLDIR)

clean : doc-clean

rebuild : clean all

.PHONY: doc doc-clean doc-test
