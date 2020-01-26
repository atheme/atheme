# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.
# Copyright (C) 2010 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2010 Jilles Tjoelker <jilles@stack.nl>
# Copyright (C) 2010-2011 JD Horelick <jdhore1@gmail.com>
# Copyright (C) 2011 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2018-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>
# Copyright (C) 2019 Janik Kleinhoff <ilbelkyr@shalture.org>

include ../../extra.mk

MODULE = operserv
SRCS   =                    \
    akill.c                 \
    clearchan.c             \
    clones.c                \
    compare.c               \
    genhash.c               \
    greplog.c               \
    help.c                  \
    identify.c              \
    ignore.c                \
    info.c                  \
    inject.c                \
    jupe.c                  \
    main.c                  \
    mode.c                  \
    modinspect.c            \
    modlist.c               \
    modload.c               \
    modreload.c             \
    modunload.c             \
    noop.c                  \
    rakill.c                \
    raw.c                   \
    readonly.c              \
    rehash.c                \
    restart.c               \
    rmatch.c                \
    rnc.c                   \
    rwatch.c                \
    set.c                   \
    set_akicktime.c         \
    set_chanexpire.c        \
    set_commitinterval.c    \
    set_core.c              \
    set_enforceprefix.c     \
    set_klinetime.c         \
    set_maxchanacs.c        \
    set_maxchans.c          \
    set_maxfounders.c       \
    set_maxlogins.c         \
    set_maxnicks.c          \
    set_maxusers.c          \
    set_mdlimit.c           \
    set_nickexpire.c        \
    set_recontime.c         \
    set_spam.c              \
    sgline.c                \
    shutdown.c              \
    soper.c                 \
    specs.c                 \
    sqline.c                \
    update.c                \
    uptime.c

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += -lathemecore
