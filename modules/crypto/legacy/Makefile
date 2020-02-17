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

include ../../../extra.mk

MODULE = crypto
SRCS   =                            \
    anope-enc-sha256.c              \
    base64.c                        \
    crypt3-des.c                    \
    crypt3-md5.c                    \
    ircservices.c                   \
    rawmd5.c                        \
    rawsha1.c                       \
    rawsha2-256.c                   \
    rawsha2-512.c                   \

include ../../../buildsys.mk
include ../../../buildsys.module.mk

CPPFLAGS += -I../../../include
LDFLAGS  += -L../../../libathemecore
CFLAGS   += ${LIBCRYPT_CFLAGS}
LIBS     += ${LIBCRYPT_LIBS} -lathemecore
