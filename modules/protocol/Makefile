# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.
# Copyright (C) 2010-2012 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2010 Jilles Tjoelker <jilles@stack.nl>
# Copyright (C) 2011 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2012 JD Horelick <jdhore1@gmail.com>
# Copyright (C) 2013 Sam Dodrill <shadow.h511@gmail.com>
# Copyright (C) 2014 Lee Starnes <starnelt@dukes.jmu.edu>
# Copyright (C) 2015-2016 Max Teufel <max@teufelsnetz.com>
# Copyright (C) 2016 Austin Ellis <siniStar@IRC4Fun.net>
# Copyright (C) 2017 Ben <ben@mrbenc.com>
# Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>

include ../../extra.mk

MODULE = protocol
SRCS   =                    \
    asuka.c                 \
    bahamut.c               \
    base36uid.c             \
    charybdis.c             \
    chatircd1.1.c           \
    elemental-ircd.c        \
    inspircd.c              \
    solanum.c               \
    ircnet.c                \
    mixin_nohalfops.c       \
    mixin_noholdnick.c      \
    mixin_noowner.c         \
    mixin_noprotect.c       \
    nefarious.c             \
    ngircd.c                \
    p10-generic.c           \
    ratbox.c                \
    ts6-generic.c           \
    unreal.c                \
    unreal4.c

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += -lathemecore
