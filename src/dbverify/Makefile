# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2012 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2018-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>

include ../../extra.mk

PROG = ${PACKAGE_TARNAME}-dbverify${PROG_SUFFIX}
SRCS = main.c

include ../../buildsys.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += -lathemecore

build: all
