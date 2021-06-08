# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.
# Copyright (C) 2010-2011 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2010 Jilles Tjoelker <jilles@stack.nl>
# Copyright (C) 2010 JD Horelick <jdhore1@gmail.com>
# Copyright (C) 2011 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>

include ../../extra.mk

MODULE = gameserv
SRCS   =            \
    dice.c          \
    eightball.c     \
    gamecalc.c      \
    help.c          \
    lottery.c       \
    main.c          \
    namegen.c       \
    rps.c

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += ${LIBMATH_LIBS} -lathemecore
