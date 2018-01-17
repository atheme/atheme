# Copyright (c) 2010 Atheme Development Group
# Rights to this code are documented in doc/LICENSE.

PLUGIN		= main$(PLUGIN_SUFFIX)
SRCS		= fix.c gather.c main.c

include ../../extra.mk
include ../../buildsys.mk

plugindir	= $(MODDIR)/modules/chanfix

CPPFLAGS	+= -I../../include
LIBS		+= -L../../libathemecore -lathemecore

CFLAGS		+= $(PLUGIN_CFLAGS)
LDFLAGS		+= $(PLUGIN_LDFLAGS)
