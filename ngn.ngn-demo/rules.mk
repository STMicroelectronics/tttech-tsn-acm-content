###############################################################################
#
# rules .mk provides common rules for building various artifacts
RULESMK_VERSION=1.1.8
# It supports the following make targets:
#   'make all': compiles and links the source code
#   'make install': installs binaries and header files (if applicable)
#   'make clean': clean all build step result (not the installed files!)
#   'make doc': generate documentation (if configured to do so)
#   'make install-doc': install the documentation (usually to /usr/share/doc)
#   'make cppcheck': call cppcheck for the source files
# for kernel modules only:
#   'make modules': build the kernel module
#   'make modules_install': install the kernel module
#   'make coccicheck': run coccinelle tests on kernel module
#
# Makefile variables used to control rule.mk
#
# MODULE (mandatory)
#  set binary to be build. Without extension a binary wil be built, *.so
#  builds a dynamic library, .a a static library and .ko a kernel module
#
# SRCDIRS
#  list of source directories to be taken into account, Defaults to 'src'.
#
# INCDIRS
#  list of include directories to be taken into account, Defaults to 'include'.
#  For libraries these directories contain the header files to will be
#  installed.
#
# DESTDIR
#  must be set if you want to install into $(DESTDIR), so e.g. Yocto recipes
#  usually simply contain 'oe_runmake install DESTDIR=${D}' in their
#  do_install task.
#
# Install related path variables
#  To support usual build environments the following variable are used to
#  install artifacts accordingl, usually these must not be changed:
#    prefix
#    bindir
#    libdir
#    includedir
#    datadir
#    docdir
#    infodir
#
# LIBSOVERSION (optional)
#  set library version number to get appropriately link dynamic library files
#  e.g. LIBSOVERSION=3.4.23
#  would install <libname>.so.3.4.23 and create links
#    <libname>.so     -> <libname>.so.3.4.23
#    <libname>.so.3   -> <libname>.so.3.4.23
#    <libname>.so.3.4 -> <libname>.so.3.4.23
#  If derived from GIT_VERSION (see GIT_VERSION_PATTERN below) using tags
#  in a X.Y.Z format, you might want to strip off git's extra information
#  using LIBSOVERSION = $(firstword $(subst -,$(space),$(GIT_VERSION)))
#
# PKGCONFIG (optional)
#  If set, it must be set to a pkgconfig template file, which gets its paths
#  adapted during install
#
# STATICLIBS  (optional)
#  list  of predefined static libraries which will be used at link time
#  Build rules for such libraries must be included in the calling makefile.
#
# CROSS_COMPILE (optional)
#  cross compile prefix for GNU toolchain; set when crosscompiling.
#
# GIT-VERSION-GEN (optional)
#  set to git version generator helper script if you want the GIT-VERSION
#  macro set at compile time.
#
# GIT-VERSION-PATTERN (optional)
#  must be set together with GIT-VERSION-GEN and specifices the regexp pattern
#  used to search for respective tags. This sets the respective GIT_VERSION
#  as C-Compiler macro.
#
# DEBUG (optional)
#  If set the respective value is set as DEBUG define (e.g. a debug level)
#  during compile in the CFLAGS
#
# DOXYFILE (optional)
#  if set to a respective doxygen file the required documentation will be
#  generated
#
# CPPCHECK_OUTPUT (optional)
#  set cppcheck outpout filename. It generates a XML output if this is a
#  *.xml file, text output otherwise
#  defaults to build/cppcheck.xml
#
# KDIR (optinal)
#  predefined directory with kernel sources to override default settings
#
# KBUILD_EXTRA_SYMBOLS (optional)
#  add list of additional symbol files (Module.symvers) when depending on
#  additional kernel modules
#
# EXTRA_CFLAGS
#  Set additional compile flags for kernel modules here, e.g. include path of
#  another module the module depends on
#
# Remark:
#  Setting V=1 (verbose) will turn of silent mode and show all the commands
#  issued, e.g. 'make V=1 all'
###############################################################################

###############################################################################
# helper macros
comma:= ,
empty:=
space:= $(empty) $(empty)
define newline


endef
streq = $(if $(subst x$1,,x$2)$(subst x$2,,x$1),$(false),$(true))
major=$(word 1,$(subst .,$(space),$1))
minor=$(word 2,$(subst .,$(space),$1))
patchlevel=$(word 3,$(subst .,$(space),$1))
echo-cmd=$(if $V,$2,@echo $1 && $2)
component-dir=$(notdir $(patsubst %/,%,$(dir $(abspath $1))))
###############################################################################

# do not pring diretory changes
MAKEFLAGS += --no-print-directory

# detect type of result via extension and prefix and set respective variable
# for the filename with its relative path
EXECUTABLE = $(if $(suffix $(MODULE)),,$(BUILDDIR)/$(MODULE))
STATLIB = $(if $(filter lib%.a,$(MODULE)),$(BUILDDIR)/$(MODULE))
DYNLIB = $(if $(filter lib%.so,$(MODULE)),$(BUILDDIR)/$(MODULE))
KMODULE = $(if $(filter %.ko,$(MODULE)),$(BUILDDIR)/$(MODULE))

# preset tool variables
# but do not preset for kernel module build
ifeq ($(KMODULE),)
CC ?= $(CROSS_COMPILE)gcc
CXX ?= $(CROSS_COMPILE)g++
LD ?= $(CROSS_COMPILE)ld
AS ?= $(CROSS_COMPILE)as
AR ?= $(CROSS_COMPILE)ar
RANLIB ?= $(CROSS_COMPILE)ranlib
NM ?= $(CROSS_COMPILE)nm
READELF ?= $(CROSS_COMPILE)readelf
STRIP ?= $(CROSS_COMPILE)strip
STRINGS ?= $(CROSS_COMPILE)strings
OBJCOPY ?= $(CROSS_COMPILE)objcopy
OBJDUMP ?= $(CROSS_COMPILE)objdump

# handle debug build centrally
ifneq ($(DEBUG),)
CFLAGS += -g -Og -DDEBUG=$(DEBUG)
else
CFLAGS += -O2
endif # DEBUG
endif # KMODULE

# determine target architecture from compiler used
TARGET_ARCH ?= $(shell $(CC) -dumpmachine)

# 'src' is the only default directory for sources
SRCDIRS ?= src
# 'include' is the only default directory for headers
INCDIRS ?= include

# this enables building on multiple target architectures without interference
BUILDDIR ?= build/$(TARGET_ARCH)

# cppcheck output
CPPCHECK_OUTPUT ?= build/cppcheck.xml

# all source and include directories might contain header files
CFLAGS += $(foreach srcdir,$(SRCDIRS),-I$(srcdir))
CFLAGS += $(foreach incdir,$(INCDIRS),-I$(incdir))

CSOURCES = $(foreach srcdir,$(SRCDIRS),$(wildcard $(srcdir)/*.c))
COBJECTS = $(foreach srcdir,$(SRCDIRS),$(patsubst $(srcdir)/%.c, $(BUILDDIR)/$(srcdir)/%.o, $(wildcard $(srcdir)/*.c)))
CHEADERS = $(foreach incdir,$(INCDIRS),$(wildcard $(incdir)/*.h))

###############################################################################
# dependency generation support
#
# store dependency files within BUILDDIR
DEPDIR = $(BUILDDIR)
CDEPS = $(foreach srcdir,$(SRCDIRS),$(patsubst $(srcdir)/%.c, $(DEPDIR)/$(srcdir)/%.d, $(wildcard $(srcdir)/*.c)))
# flags for automatic dependency generation, see compile rule
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
# pull in dependency info for *existing* *.o files
-include $(CDEPS)

# rule with an empty recipe, so that make won’t fail
# if the dependency file doesn’t exist.
$(DEPDIR)/%.d: ;

# Mark the dependency files precious to make, so they won’t be automatically
# deleted as intermediate files.
.PRECIOUS: $(DEPDIR)/%.d
###############################################################################

# set defaults
prefix ?= /usr
bindir ?= $(prefix)/bin
libdir ?= $(prefix)/lib
includedir ?= $(prefix)/include
datadir ?= $(prefix)/share
docdir ?= $(datadir)/doc
infodir ?= $(datadir)/info

# get doxygen
DOXYGEN ?= $(shell which doxygen)
PDFLATEX ?= $(shell which pdflatex)

###############################################################################
# git version file generation
#
ifneq ($(GIT-VERSION-GEN),)

ifeq ($(GIT-VERSION-PATTERN),)
$(error GIT-VERSION-PATTERN not set)
endif

GIT-VERSION-FILE = $(BUILDDIR)/GIT-VERSION-FILE
$(GIT-VERSION-FILE): FORCE
	@mkdir -p $(@D)
	@$(GIT-VERSION-GEN) $(GIT-VERSION-FILE) "$(GIT-VERSION-PATTERN)"

-include $(GIT-VERSION-FILE)
CFLAGS += -DGIT_VERSION='$(GIT_VERSION)'

endif # GIT-VERSION-GEN
###############################################################################

ifneq ($(LIBSOVERSION),)
LIBSOVERSION_SUFFIX = .$(LIBSOVERSION)
endif

###
# use FORCE target as depedency to force execution of a rule
PHONY += FORCE
FORCE:


###############################################################################
# C compile rule with dependency generation
#
$(BUILDDIR)/%.o: %.c $(DEPDIR)/%.d $(BUILDDIR)/.cflags.d $(BUILDDIR)/.cppflags.d
	@mkdir -p $(@D)
	$(call echo-cmd,"  CC $(notdir $@)",				\
		$(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c $< -o $@ &&	\
		mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d &&			\
		touch $@)
###############################################################################

###############################################################################
# kernel module build support
#
ifneq ($(KMODULE),)
ifeq ($(KERNELRELEASE),)

# either use KDIR or default to local kernel otherwise
# set by Yocto's module-base.bbclass
ifneq ($(KDIR),)
KBUILD_OUTPUT := $(KDIR)
endif
KBUILD_OUTPUT ?= /lib/modules/$(shell uname -r)/build

# assert Kbuild, needed by kernel build system
# remark: this also avoids re-entering this makefile by kernel build
ifeq ($(wildcard Kbuild),)
$(error Kbuild file is missing)
endif 

modules: $(KMODULE)
	@echo "  DONE $(call component-dir,$(MODULE))"

# delegate build to kernel build system
$(KMODULE): $(KBUILD_OUTPUT)/Module.symvers Kbuild $(CSOURCES) $(CHEADERS)
	$(call echo-cmd,"  MAKE $(notdir $@)",				\
		$(MAKE) -C $(KBUILD_OUTPUT) M=$(abspath .)		\
			GIT_VERSION="$(GIT_VERSION)"			\
			EXTRA_CFLAGS="$(EXTRA_CFLAGS)			\
				      $(foreach srcdir,$(SRCDIRS),-I$(abspath $(srcdir)))	\
				      $(foreach incdir,$(INCDIRS),-I$(abspath $(incdir)))"	\
			KBUILD_EXTRA_SYMBOLS=$(KBUILD_EXTRA_SYMBOLS)	\
			modules)

modules_install:
	$(call echo-cmd,"  MAKE $(notdir $@)",	\
		$(MAKE) -C $(KBUILD_OUTPUT) M=$(abspath .) $@)

coccicheck:
	$(call echo-cmd,"  MAKE $(notdir $@)",	\
		$(MAKE) -C $(KBUILD_OUTPUT) M=$(abspath .) MODE=report $@)

endif # KERNELRELEASE
else
.DEFAULT_GOAL := all
endif # KMODULE
###############################################################################


###############################################################################
# executable build support
#
ifneq ($(EXECUTABLE),)
BUILDABLES += $(EXECUTABLE)
INSTALLABLES += $(DESTDIR)$(bindir)/$(MODULE)
#install: $(DESTDIR)$(bindir)/$(MODULE)

$(EXECUTABLE): $(COBJECTS) $(STATICLIBS) $(MODULEDEPS) $(BUILDDIR)/.ldflags.d
	@mkdir -p $(@D)
	$(call echo-cmd,"  LD $(notdir $@)",	\
		$(CC) $(COBJECTS) $(STATICLIBS) $(LDFLAGS) -o $@)

$(DESTDIR)$(bindir)/$(MODULE): $(EXECUTABLE)
	$(call echo-cmd,"  INSTALL $(notdir $@)",	\
		install -d $(@D) && install -m 0755 $< $(@D))

###############################################################################
else
###############################################################################
# library build support
#
INSTALLHEADERS = $(addprefix $(DESTDIR)$(includedir)/$(basename $(MODULE))/,$(notdir $(CHEADERS)))

BUILDABLES += $(STATLIB) $(DYNLIB)
INSTALLABLES += $(DESTDIR)$(libdir)/$(MODULE)

ifneq ($(LIBSOVERSION),)
INSTALLABLES += $(DESTDIR)$(libdir)/$(MODULE).$(LIBSOVERSION)
BUILDABLES += $(DYNLIB).$(LIBSOVERSION)
endif

ifneq ($(call major,$(LIBSOVERSION)),)
ifneq ($(call major,$(LIBSOVERSION)),$(LIBSOVERSION))
INSTALLABLES += $(DESTDIR)$(libdir)/$(MODULE).$(call major,$(LIBSOVERSION))
BUILDABLES += $(DYNLIB).$(call major,$(LIBSOVERSION))
endif
endif

ifneq ($(call minor,$(LIBSOVERSION)),)
ifneq ($(call major,$(LIBSOVERSION)).$(call minor,$(LIBSOVERSION)),$(LIBSOVERSION))
INSTALLABLES += $(DESTDIR)$(libdir)/$(MODULE).$(call major,$(LIBSOVERSION)).$(call minor,$(LIBSOVERSION))
BUILDABLES += $(DYNLIB).$(call major,$(LIBSOVERSION)).$(call minor,$(LIBSOVERSION))
endif
endif

INSTALLABLES += $(INSTALLHEADERS)

define header_install_rule
$(DESTDIR)$(includedir)/$(basename $(MODULE))/$$(notdir $1): $1
	$$(call echo-cmd,"  INSTALL $$(notdir $$@)",	\
		install -d $$(@D) && install -m 0644 $$< $$(@D))

endef
$(foreach h,$(CHEADERS),$(eval $(call header_install_rule,$h)))

$(DESTDIR)$(libdir)/$(MODULE)%: $(DYNLIB)%
	$(call echo-cmd,"  INSTALL $(notdir $@)",	\
		install -d $(@D) && cp -P $< $(@D) && chmod 755 $(@D))

$(DESTDIR)$(libdir)/$(MODULE): $(DYNLIB)
	$(call echo-cmd,"  INSTALL $(notdir $@)",	\
		install -d $(@D) && cp -P $< $(@D) && chmod 755 $(@D))

###############################################################################
# create/update pkgconfig file (dynamic libs only)
#
ifneq ($(PKGCONFIG),)
INSTALLABLES += $(DESTDIR)$(libdir)/pkgconfig/$(notdir $(PKGCONFIG))

$(DESTDIR)$(libdir)/pkgconfig/$(notdir $(PKGCONFIG)): $(PKGCONFIG)
	$(call echo-cmd,"  PKGCONFIG $(notdir $@)", \
		mkdir -p $(@D) && \
		sed -e "s%^prefix=.*%prefix=$(prefix)%g" \
		    -e "s%^exec_prefix=.*%exec_prefix=$(prefix)%g" \
		    -e "s%^libdir=.*%libdir=$(libdir)%g" \
		    -e "s%^includedir=.*%includedir=$(includedir)%g" $< >$@)
endif # $(PKGCONFIG)
###############################################################################

endif # !$(EXECUTABLE)
###############################################################################

###############################################################################
# archive static library
#
ifneq ($(STATLIB),)
CFLAGS += -fPIC

$(STATLIB): $(COBJECTS) $(MODULEDEPS)
	@mkdir -p $(@D)
	$(call echo-cmd,"  AR $(notdir $@)",	\
		$(AR) rcs $@ $(COBJECTS))
endif
###############################################################################

###############################################################################
# link dynamic library
#
ifneq ($(DYNLIB),)
CFLAGS += -fPIC

ifneq ($(LIBSOVERSION),)
# link and soft link
$(DYNLIB).$(LIBSOVERSION): $(COBJECTS) $(STATICLIBS) $(MODULEDEPS) $(BUILDDIR)/.ldflags.d
	@mkdir -p $(@D)
	$(call echo-cmd,"  LD $(notdir $@)",							\
		$(CC) -shared									\
			-Wl$(comma)-soname$(comma)$(MODULE).$(call major,$(LIBSOVERSION))	\
			$(COBJECTS) $(STATICLIBS) $(LDFLAGS) -o $@)

$(DYNLIB): $(DYNLIB).$(LIBSOVERSION)
	$(call echo-cmd,"  LN $(notdir $@)",ln -sf $(notdir $<) $@)

$(DYNLIB).%: $(DYNLIB).$(LIBSOVERSION)
	$(call echo-cmd,"  LN $(notdir $@)",ln -sf $(notdir $<) $@)

else
# just link, no soft linking
$(DYNLIB): $(COBJECTS) $(STATICLIBS) $(MODULEDEPS) $(BUILDDIR)/.ldflags.d
	@mkdir -p $(@D)
	$(call echo-cmd,"  LD $(notdir $@)",	\
		$(CC) -shared $(COBJECTS) $(STATICLIBS) $(LDFLAGS) -o $@)
endif
endif
###############################################################################

###############################################################################
# doxygen support
#
ifneq ($(DOXYFILE),)
ifneq ($(wildcard $(DOXYFILE)),)

# Get build relevant data from doxyfile
#
# assume all paths in the doxyfile are paths set relative
# to the doxyfile path. This also easily enables a direct use of the doxyfile
# when call within its directory. This is also important when using the Eclox
# Eclipse plugin for Doxygen.
$(eval $(shell grep -E '^OUTPUT_DIRECTORY\s*=' $(DOXYFILE)))
$(eval $(shell grep -E '^GENERATE_LATEX\s*=' $(DOXYFILE)))
$(eval $(shell grep -E '^LATEX_OUTPUT\s*=' $(DOXYFILE)))
$(eval $(shell grep -E '^GENERATE_HTML\s*=' $(DOXYFILE)))
$(eval $(shell grep -E '^HTML_OUTPUT\s*=' $(DOXYFILE)))
$(eval $(shell grep -E '^HTML_FILE_EXTENSION\s*=' $(DOXYFILE)))

DOXYOUTFILEBASE = $(basename $(notdir $(DOXYFILE)))

DOXYBUILDDIR = $(dir $(DOXYFILE))$(OUTPUT_DIRECTORY)
DOXYLATEXBUILDDIR = $(DOXYBUILDDIR)/$(LATEX_OUTPUT)
DOXYHTMLBUILDDIR = $(DOXYBUILDDIR)/$(HTML_OUTPUT)

ifeq ($(GENERATE_LATEX),YES)
DOXYOUTPUT += $(DOXYLATEXBUILDDIR)/refman.tex
DOCOUTPUT += $(DOXYBUILDDIR)/$(DOXYOUTFILEBASE).pdf
endif

ifeq ($(GENERATE_HTML),YES)
DOXYOUTPUT += $(DOXYHTMLBUILDDIR)/index$(HTML_FILE_EXTENSION)
DOCOUTPUT += $(DOXYBUILDDIR)/$(DOXYOUTFILEBASE)$(HTML_FILE_EXTENSION).zip
endif

$(DOXYOUTPUT): $(DOXYFILE) $(CSOURCES) $(CHEADERS)
	@mkdir -p $(@D)
	$(call echo-cmd,"  DOXYGEN $(notdir $<)",	\
		cd $(dir $(DOXYFILE)) && 		\
		doxygen $(abspath $<) >$(abspath $(DOXYBUILDDIR)/doxygen.log))

ifeq ($(GENERATE_LATEX),YES)
$(DOXYLATEXBUILDDIR)/refman.pdf: $(DOXYLATEXBUILDDIR)/refman.tex
	$(call echo-cmd,"  PDFLATEX $(notdir $@)",	\
		$(MAKE) -C $(dir $<)			\
		>$(abspath $(DOXYBUILDDIR)/pdflatex.log) 2>&1)
	
$(DOXYBUILDDIR)/$(DOXYOUTFILEBASE).pdf: $(DOXYLATEXBUILDDIR)/refman.pdf
	$(call echo-cmd,"  CP $(notdir $@)",	\
		mkdir -p $(@D); cp $< $@)
endif # GENERATE_LATEX

ifeq ($(GENERATE_HTML),YES)
$(DOXYBUILDDIR)/$(DOXYOUTFILEBASE)$(HTML_FILE_EXTENSION).zip: $(DOXYHTMLBUILDDIR)/index$(HTML_FILE_EXTENSION)
	$(call echo-cmd,"  ZIP $(notdir $@)",	\
		mkdir -p $(@D); cd $(<D) && zip -q -r -9 $(abspath $@) *)
endif

doc: $(DOCOUTPUT)
	@echo "  DOCGEN $(call component-dir,$(MODULE))"

$(DESTDIR)$(docdir)/$(DOXYOUTFILEBASE)/%: $(DOCOUTPUT)
	$(call echo-cmd,"  INSTALL $(notdir $@)",	\
		install -d $(@D) &&			\
		install -m 0644 $< $(@D))
	
install-doc: $(DOCOUTPUT)
	$(call echo-cmd,"  INSTDOC $(call component-dir,$(MODULE))",	\
		install -d $(DESTDIR)$(docdir)/$(DOXYOUTFILEBASE) &&	\
		install -m 0644 $(wildcard $(DOXYBUILDDIR)/*.zip) $(wildcard $(DOXYBUILDDIR)/*.pdf) $(DESTDIR)$(docdir)/$(DOXYOUTFILEBASE))

endif # $(wildcard $(DOXYFILE))
endif # DOXYFILE
###############################################################################

###
# build the respective binary
all: $(BUILDABLES)
	@echo "  BUILT $(call component-dir,$(MODULE))"

install: $(INSTALLABLES)
	@echo "  INSTALLED $(call component-dir,$(MODULE))"

###############################################################################
# cppcheck section
#

# switch to xml output if requested
ifneq ($(filter %.xml,$(CPPCHECK_OUTPUT)),)
cppcheck_xml_args = --xml --xml-version=2
endif

$(CPPCHECK_OUTPUT): FORCE
	$(call echo-cmd,"  CREATE $(notdir $@)",			\
		mkdir -p $(@D);						\
		cppcheck --enable=all $(cppcheck_xml_args)		\
			--suppress=missingIncludeSystem			\
			--inline-suppr					\
			$(CPPCHECK_ARG) $(filter -I%,$(CFLAGS)) 	\
			$(filter -I%,$(CPPFLAGS)) -q 2>$@) .

cppcheck: $(CPPCHECK_OUTPUT)
	@echo "  CPPCHECK $(call component-dir,$(MODULE))"

###############################################################################
# Clean section
#
cleanup = rm -rf $(BUILDDIR);rm -rf $(DOXYBUILDDIR); rm -f $(CPPCHECK_OUTPUT)
ifneq ($(KMODULE),)
cleanup += ;rm -f Module.symvers
cleanup += ;rm -rf .tmp_versions
cleanup += ;for d in $$(find -maxdepth 1  -type d); do	\
		rm -f $$d/*.o $$d/*.ko;			\
		rm -f $$d/*.mod.c $$d/\.*.o.cmd;	\
		rm -f $$d/\.*.ko.cmd;			\
		rm -f $$d/modules.order;		\
	done
endif

clean: FORCE
	$(call echo-cmd,"  CLEAN $(call component-dir,$(MODULE))",$(cleanup))
###############################################################################

# react on compiler/linker flags change
$(BUILDDIR)/.cflags.d: FORCE
	@mkdir -p $(@D)
	@echo "$(CFLAGS)" > $@.tmp
	@[ -f $@ ] && cmp -s $@ $@.tmp || mv $@.tmp $@

$(BUILDDIR)/.cppflags.d: FORCE
	@mkdir -p $(@D)
	@echo "$(CPPFLAGS)" > $@.tmp
	@[ -f $@ ] && cmp -s $@ $@.tmp || mv $@.tmp $@

$(BUILDDIR)/.ldflags.d: FORCE
	@mkdir -p $(@D)
	@echo "$(LDFLAGS)" > $@.tmp
	@[ -f $@ ] && cmp -s $@ $@.tmp || mv $@.tmp $@

# Disable the built-in rules by writing an empty rule for .SUFFIXES:
.SUFFIXES:

$(info $(newline)INFO: Using rules.mk v$(RULESMK_VERSION))
# vim tabstop=8 shiftwidth=8 softtabstop=8 textwidth=80 noexpandtab
