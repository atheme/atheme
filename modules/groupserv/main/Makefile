# Copyright (c) 2010 Atheme Development Group
# Rights to this code are documented in doc/LICENSE.

PLUGIN = main$(PLUGIN_SUFFIX)

SRCS = main.c database.c groupserv.c hooks.c validation.c

include ../../../extra.mk
include ../../../buildsys.mk

plugindir = $(MODDIR)/modules/groupserv

CPPFLAGS += -I../../../include -I..
CFLAGS += $(PLUGIN_CFLAGS)
LDFLAGS += $(PLUGIN_LDFLAGS)
LIBS +=	-L../../../libathemecore -lathemecore ${LDFLAGS_RPATH}

