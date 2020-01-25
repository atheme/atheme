# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2010-2014 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2012 Bryan Drewery <bryan@shatow.net>
# Copyright (C) 2015 Mantas Mikulėnas <grawity@gmail.com>
# Copyright (C) 2017-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>

include ../extra.mk

SHARED_LIB = ${LIB_PREFIX}athemecore${LIB_SUFFIX}
LIB_MAJOR  = 1
LIB_MINOR  = 0

SRCS =                              \
    ${QRCODE_COND_C}                \
    account.c                       \
    atheme.c                        \
    auth.c                          \
    authcookie.c                    \
    base64.c                        \
    channels.c                      \
    cidr.c                          \
    cmode.c                         \
    commandhelp.c                   \
    commandtree.c                   \
    conf.c                          \
    confprocess.c                   \
    connection.c                    \
    crypto.c                        \
    ctcp-common.c                   \
    culture.c                       \
    database_backend.c              \
    datastream.c                    \
    digest_direct_md5.c             \
    digest_direct_sha1.c            \
    digest_direct_sha2.c            \
    digest_frontend.c               \
    digest_testsuite.c              \
    eksblowfish.c                   \
    entity.c                        \
    flags.c                         \
    function.c                      \
    hook.c                          \
    linker.c                        \
    logger.c                        \
    match.c                         \
    memory_frontend.c               \
    module.c                        \
    node.c                          \
    object.c                        \
    packet.c                        \
    phandler.c                      \
    pmodule.c                       \
    privs.c                         \
    ptasks.c                        \
    random_frontend.c               \
    send.c                          \
    servers.c                       \
    services.c                      \
    servtree.c                      \
    sharedheap.c                    \
    signal.c                        \
    string.c                        \
    strshare.c                      \
    svsignore.c                     \
    table.c                         \
    template.c                      \
    tokenize.c                      \
    ubase64.c                       \
    uid.c                           \
    uplink.c                        \
    users.c                         \
    version.c

include ../buildsys.mk

CPPFLAGS += -I../include

CFLAGS +=                           \
    ${LIBCRYPTO_CFLAGS}             \
    ${LIBGCRYPT_CFLAGS}             \
    ${LIBMBEDCRYPTO_CFLAGS}         \
    ${LIBNETTLE_CFLAGS}             \
    ${LIBPCRE_CFLAGS}               \
    ${LIBQRENCODE_CFLAGS}           \
    ${LIBSODIUM_CFLAGS}             \
    ${LIB_CFLAGS}

LIBS +=                             \
    ${LIBCRYPTO_LIBS}               \
    ${LIBGCRYPT_LIBS}               \
    ${LIBMBEDCRYPTO_LIBS}           \
    ${LIBNETTLE_LIBS}               \
    ${LIBPCRE_LIBS}                 \
    ${LIBQRENCODE_LIBS}             \
    ${LIBSODIUM_LIBS}               \
    ${LIBDL_LIBS}                   \
    ${LIBSOCKET_LIBS}

build: depend all
