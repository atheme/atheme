# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.
# Copyright (C) 2010-2014 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2010 Jilles Tjoelker <jilles@stack.nl>
# Copyright (C) 2011 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2013 Elizabeth Myers <elizabeth@interlinked.me>
# Copyright (C) 2017-2020 Aaron M. D. Jones <me@aaronmdjones.net>

include ../../extra.mk

MODULE = saslserv
SRCS   =                            \
    authcookie.c                    \
    ecdh-x25519-challenge.c         \
    ecdsa-nist256p-challenge.c      \
    external.c                      \
    main.c                          \
    plain.c                         \
    scram.c

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore

CFLAGS +=                           \
    ${LIBCRYPTO_CFLAGS}             \
    ${LIBMBEDCRYPTO_CFLAGS}         \
    ${LIBNETTLE_CFLAGS}             \
    ${LIBSODIUM_CFLAGS}

LIBS +=                             \
    ${LIBCRYPTO_LIBS}               \
    ${LIBMBEDCRYPTO_LIBS}           \
    ${LIBNETTLE_LIBS}               \
    ${LIBSODIUM_LIBS}               \
    -lathemecore
