# SPDX-License-Identifier: CC0-1.0
# SPDX-URL: https://spdx.org/licenses/CC0-1.0.html
#
# Copyright (C) 2019-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>

include ../../extra.mk

PROG = ${PACKAGE_TARNAME}-crypto-benchmark${PROG_SUFFIX}
SRCS = benchmark.c main.c optimal.c selftests.c

include ../../buildsys.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore

CFLAGS +=                   \
    ${LIBARGON2_CFLAGS}     \
    ${LIBSODIUM_CFLAGS}

LIBS +=                     \
    ${LIBARGON2_LIBS}       \
    ${LIBSODIUM_LIBS}       \
    ${CLOCK_GETTIME_LIBS}   \
    -lathemecore

build: all
