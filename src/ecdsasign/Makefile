# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2013 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2017-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>

include ../../extra.mk

PROG = ${PACKAGE_TARNAME}-ecdsasign${PROG_SUFFIX}
SRCS = main.c

include ../../buildsys.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
CFLAGS   += ${LIBCRYPTO_CFLAGS}
LIBS     += ${LIBCRYPTO_LIBS} -lathemecore

build: all
