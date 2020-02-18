# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.
# Copyright (C) 2010-2012 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2010 Jilles Tjoelker <jilles@stack.nl>
# Copyright (C) 2011 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2012 Elly Fong-Jones <elly@leptoquark.net>
# Copyright (C) 2014 Mantas Mikulėnas <grawity@gmail.com>
# Copyright (C) 2015-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>

include ../../extra.mk

SUBDIRS = ${LEGACY_PWCRYPTO_COND_D}
MODULE  = crypto
SRCS    =                           \
    argon2.c                        \
    bcrypt.c                        \
    crypt3-sha2-256.c               \
    crypt3-sha2-512.c               \
    main.c                          \
    pbkdf2.c                        \
    pbkdf2v2.c                      \
    scrypt.c                        \

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore

CFLAGS +=                           \
    ${LIBARGON2_CFLAGS}             \
    ${LIBCRYPT_CFLAGS}              \
    ${LIBIDN_CFLAGS}                \
    ${LIBSODIUM_CFLAGS}

LIBS +=                             \
    ${LIBARGON2_LIBS}               \
    ${LIBCRYPT_LIBS}                \
    ${LIBIDN_LIBS}                  \
    ${LIBSODIUM_LIBS}               \
    -lathemecore
