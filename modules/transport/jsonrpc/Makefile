# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2014 Errietta Kostala <errietta@errietta.me>
# Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>

include ../../../extra.mk

plugindir = ${MODDIR}/modules/transport
PLUGIN    = jsonrpc${PLUGIN_SUFFIX}
SRCS      = jsonrpclib.c main.c

include ../../../buildsys.mk

CPPFLAGS += -I../../../include
LDFLAGS  += -L../../../libathemecore
LIBS     += -lathemecore
