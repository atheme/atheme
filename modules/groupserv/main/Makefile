# Copyright (c) 2010 Atheme Development Group
# Rights to this code are documented in doc/LICENSE.

PLUGIN		= main$(PLUGIN_SUFFIX)
SRCS		= database.c groupserv.c hooks.c main.c validation.c

include ../../../extra.mk
include ../../../buildsys.mk

plugindir	= $(MODDIR)/modules/groupserv

CPPFLAGS	+= -I../../../include -I..
LDFLAGS		+= -L../../../libathemecore
LIBS		+= -lathemecore

CFLAGS		+= $(PLUGIN_CFLAGS)
LDFLAGS		+= $(PLUGIN_LDFLAGS)
