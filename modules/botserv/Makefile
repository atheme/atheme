# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.
# Copyright (C) 2010 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2010 Jilles Tjoelker <jilles@stack.nl>
# Copyright (C) 2010 JD Horelick <jdhore1@gmail.com>
# Copyright (C) 2011 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2016 Austin Ellis <siniStar@IRC4Fun.net>
# Copyright (C) 2018-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>

include ../../extra.mk

MODULE = botserv
SRCS   =                    \
    bottalk.c               \
    help.c                  \
    info.c                  \
    main.c                  \
    set.c                   \
    set_core.c              \
    set_fantasy.c           \
    set_nobot.c             \
    set_private.c           \
    set_saycaller.c

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += -lathemecore
