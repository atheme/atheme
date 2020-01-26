# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2010 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2018-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>

include ../../extra.mk

plugindir = ${MODDIR}/modules/chanfix
PLUGIN    = main${PLUGIN_SUFFIX}
SRCS      =                 \
    fix.c                   \
    gather.c                \
    main.c

include ../../buildsys.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += -lathemecore

CFLAGS   += ${PLUGIN_CFLAGS}
LDFLAGS  += ${PLUGIN_LDFLAGS}
