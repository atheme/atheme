# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2010 Atheme Project (http://atheme.org/)

PLUGIN		= main$(PLUGIN_SUFFIX)
SRCS		= fix.c gather.c main.c

include ../../extra.mk
include ../../buildsys.mk

plugindir	= $(MODDIR)/modules/chanfix

CPPFLAGS	+= -I../../include
LDFLAGS		+= -L../../libathemecore
LIBS		+= -lathemecore

CFLAGS		+= $(PLUGIN_CFLAGS)
LDFLAGS		+= $(PLUGIN_LDFLAGS)
