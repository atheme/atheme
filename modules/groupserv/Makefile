# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2010 Elly <elly@leptoquark.net>
# Copyright (C) 2010-2014 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2010-2012 JD Horelick <jdhore1@gmail.com>
# Copyright (C) 2010-2011 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>

include ../../extra.mk

MODULE  = groupserv
SUBDIRS = main
SRCS    =                   \
    acsnolimit.c            \
    drop.c                  \
    fflags.c                \
    flags.c                 \
    help.c                  \
    info.c                  \
    invite.c                \
    join.c                  \
    list.c                  \
    listchans.c             \
    register.c              \
    regnolimit.c            \
    set.c                   \
    set_channel.c           \
    set_description.c       \
    set_email.c             \
    set_groupname.c         \
    set_joinflags.c         \
    set_open.c              \
    set_public.c            \
    set_url.c

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += -lathemecore
