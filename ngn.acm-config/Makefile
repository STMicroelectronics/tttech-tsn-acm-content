# name of the final target
MODULE = libacmconfig.so
# strip off git extra info
#LIBSOVERSION = $(firstword $(subst -,$(space),$(GIT_VERSION)))
LIBSOVERSION = $(EXTERNAL_LIBRARY_VERSION)
PKGCONFIG=libacmconfig.pc

# default flags
# use stack protector and FORTIFY_SOURCE: see https://idea.popcount.org/2013-08-15-fortify_source/
CFLAGS += -Werror -Wall -pthread -fvisibility=hidden -D_FORTIFY_SOURCE=2 -fstack-protector
CPPFLAGS += -Iinclude
LDFLAGS += -fvisibility=hidden -pthread
DEBUG = 1

# add any static library here (must be in library search path)
STATICLIBS +=
DOXYFILE = doc/libacmconfig.doxyfile

SRCDIRS = src
INCDIRS = include

# use git version
GIT-VERSION-GEN = ./GIT-VERSION-GEN
# use pattern according to glob(7)
GIT-VERSION-PATTERN = "[0-9]*.[0-9]*.[0-9]*"

# skip build and vendor directories
# and ignore unused functions and missing includes (i.e. mocks) in test files
CPPCHECK_ARG += -ibuild -ivendor
CPPCHECK_ARG += --suppress=unusedFunction:test/test/*
CPPCHECK_ARG += --suppress=missingInclude:test/test/*

# the magic stuff is in here ..
include rules.mk

# definition of the git version as a string
CFLAGS += -DGIT_VERSION_STR=\"$(GIT_VERSION)\"
