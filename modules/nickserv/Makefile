# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.
# Copyright (C) 2010-2012 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2010-2012 Jilles Tjoelker <jilles@stack.nl>
# Copyright (C) 2010-2011 JD Horelick <jdhore1@gmail.com>
# Copyright (C) 2010 Elly <elly@leptoquark.net>
# Copyright (C) 2010-2011 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2012 Nathan Phillip Brink <binki@gentoo.org>
# Copyright (C) 2014 Errietta Kostala <errietta@errietta.me>
# Copyright (C) 2014-2015 Max Teufel <max@teufelsnetz.com>
# Copyright (C) 2016 Austin Ellis <siniStar@IRC4Fun.net>
# Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>

include ../../extra.mk

MODULE = nickserv
SRCS   =                    \
    access.c                \
    badmail.c               \
    cert.c                  \
    drop.c                  \
    enforce.c               \
    freeze.c                \
    ghost.c                 \
    group.c                 \
    help.c                  \
    hold.c                  \
    identify.c              \
    info.c                  \
    info_lastquit.c         \
    list.c                  \
    listchans.c             \
    listgroups.c            \
    listlogins.c            \
    listmail.c              \
    listownmail.c           \
    login.c                 \
    loginnolimit.c          \
    logout.c                \
    main.c                  \
    mark.c                  \
    multimark.c             \
    pwquality.c             \
    register.c              \
    regnolimit.c            \
    resetpass.c             \
    restrict.c              \
    return.c                \
    sendpass.c              \
    sendpass_user.c         \
    set.c                   \
    set_accountname.c       \
    set_core.c              \
    set_email.c             \
    set_emailmemos.c        \
    set_enforcetime.c       \
    set_hidelastlogin.c     \
    set_hidemail.c          \
    set_language.c          \
    set_nevergroup.c        \
    set_neverop.c           \
    set_nogreet.c           \
    set_nomemo.c            \
    set_noop.c              \
    set_nopassword.c        \
    set_password.c          \
    set_private.c           \
    set_privmsg.c           \
    set_property.c          \
    set_pubkey.c            \
    set_quietchg.c          \
    setpass.c               \
    status.c                \
    taxonomy.c              \
    vacation.c              \
    verify.c                \
    vhost.c                 \
    waitreg.c

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore

CFLAGS +=                   \
    ${LIBCRACK_CFLAGS}      \
    ${LIBPASSWDQC_CFLAGS}

LIBS +=                     \
    ${LIBCRACK_LIBS}        \
    ${LIBPASSWDQC_LIBS}     \
    -lathemecore
