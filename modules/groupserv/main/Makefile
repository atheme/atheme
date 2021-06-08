# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2010 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>

include ../../../extra.mk

plugindir = ${MODDIR}/modules/groupserv
PLUGIN    = main${PLUGIN_SUFFIX}
SRCS      =         \
    database.c      \
    groupserv.c     \
    hooks.c         \
    main.c          \
    validation.c

include ../../../buildsys.mk

CPPFLAGS += -I../../../include -I..
LDFLAGS  += -L../../../libathemecore
LIBS     += -lathemecore
