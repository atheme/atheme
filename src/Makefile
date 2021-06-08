# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2010-2013 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2010 Jilles Tjoelker <jilles@stack.nl>
# Copyright (C) 2010 Elly <elly@leptoquark.net>
# Copyright (C) 2010 JD Horelick <jdhore1@gmail.com>
# Copyright (C) 2017-2020 Aaron M. D. Jones <me@aaronmdjones.net>

include ../extra.mk

SUBDIRS =                           \
    ${CRYPTO_BENCHMARK_COND_D}      \
    ${ECDH_X25519_TOOL_COND_D}      \
    ${ECDSA_NIST256P_TOOLS_COND_D}  \
    dbverify                        \
    services

include ../buildsys.mk
