# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.
# Copyright (C) 2010-2015 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2010 Jilles Tjoelker <jilles@stack.nl>
# Copyright (C) 2010-2011 JD Horelick <jdhore1@gmail.com>
# Copyright (C) 2011 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2014 Lee Starnes <starnelt@dukes.jmu.edu>
# Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>

include ../../extra.mk

MODULE = chanserv
SRCS   =                    \
    access.c                \
    akick.c                 \
    antiflood.c             \
    ban.c                   \
    bansearch.c             \
    clear.c                 \
    clear_akicks.c          \
    clear_bans.c            \
    clear_flags.c           \
    clear_users.c           \
    clone.c                 \
    close.c                 \
    count.c                 \
    drop.c                  \
    fflags.c                \
    flags.c                 \
    ftransfer.c             \
    getkey.c                \
    halfop.c                \
    help.c                  \
    hold.c                  \
    info.c                  \
    invite.c                \
    kick.c                  \
    list.c                  \
    main.c                  \
    mark.c                  \
    moderate.c              \
    op.c                    \
    owner.c                 \
    protect.c               \
    quiet.c                 \
    recover.c               \
    register.c              \
    set.c                   \
    set_core.c              \
    set_email.c             \
    set_entrymsg.c          \
    set_fantasy.c           \
    set_gameserv.c          \
    set_guard.c             \
    set_keeptopic.c         \
    set_limitflags.c        \
    set_mlock.c             \
    set_prefix.c            \
    set_private.c           \
    set_property.c          \
    set_pubacl.c            \
    set_restricted.c        \
    set_secure.c            \
    set_topiclock.c         \
    set_url.c               \
    set_verbose.c           \
    status.c                \
    successor_acl.c         \
    sync.c                  \
    taxonomy.c              \
    template.c              \
    topic.c                 \
    unban_self.c            \
    version.c               \
    voice.c                 \
    why.c                   \
    xop.c

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += -lathemecore
