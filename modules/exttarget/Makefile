# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2011 JD Horelick <jdhore1@gmail.com>
# Copyright (C) 2014 Mantas Mikulėnas <grawity@gmail.com>
# Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>

include ../../extra.mk

MODULE = exttarget
SRCS   =                    \
    chanacs.c               \
    channel.c               \
    main.c                  \
    oper.c                  \
    registered.c            \
    server.c

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += -lathemecore
