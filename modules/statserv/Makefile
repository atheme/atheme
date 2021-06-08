# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2011 Alyx <alyx@sporksmoo.net>
# Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>

include ../../extra.mk

MODULE = statserv
SRCS   =            \
    channel.c       \
    main.c          \
    netsplit.c      \
    pwhashes.c      \
    server.c

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += -lathemecore
