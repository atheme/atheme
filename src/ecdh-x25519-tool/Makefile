# SPDX-License-Identifier: CC0-1.0
# SPDX-URL: https://spdx.org/licenses/CC0-1.0.html
#
# Copyright (C) 2019-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>

include ../../extra.mk

PROG = ${PACKAGE_TARNAME}-ecdh-x25519-tool${PROG_SUFFIX}
SRCS = main.c qrcode.c

include ../../buildsys.mk

CPPFLAGS += -I../../include -I../../modules/saslserv
LDFLAGS  += -L../../libathemecore

CFLAGS +=                   \
    ${LIBCRYPTO_CFLAGS}     \
    ${LIBMBEDCRYPTO_CFLAGS} \
    ${LIBNETTLE_CFLAGS}     \
    ${LIBQRENCODE_CFLAGS}   \
    ${LIBSODIUM_CFLAGS}

LIBS +=                     \
    ${LIBCRYPTO_LIBS}       \
    ${LIBMBEDCRYPTO_LIBS}   \
    ${LIBNETTLE_LIBS}       \
    ${LIBQRENCODE_LIBS}     \
    ${LIBSODIUM_LIBS}       \
    -lathemecore

build: all
