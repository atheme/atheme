# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2011-2012 William Pitcock <nenolod@dereferenced.org>

include ../../extra.mk

PROG_NOINST = ${PACKAGE_TARNAME}-footprint${PROG_SUFFIX}
SRCS        = main.c

include ../../buildsys.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += -lathemecore

build: all
