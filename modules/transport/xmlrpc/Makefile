# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2010 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>

include ../../../extra.mk

plugindir = ${MODDIR}/modules/transport
PLUGIN    = xmlrpc${PLUGIN_SUFFIX}
SRCS      = main.c xmlrpclib.c

include ../../../buildsys.mk

CPPFLAGS += -I../../../include
LDFLAGS  += -L../../../libathemecore
LIBS     += -lathemecore
