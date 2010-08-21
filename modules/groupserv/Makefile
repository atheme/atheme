# Copyright (c) 2010 Atheme Development Group
# Rights to this code are documented in doc/LICENSE.

PLUGIN = main$(PLUGIN_SUFFIX)

SRCS = main.c groupserv.c

include ../../extra.mk
include ../../buildsys.mk

plugindir = $(MODDIR)/modules/groupserv

CPPFLAGS += -I../../include
CFLAGS += $(PLUGIN_CFLAGS)
LDFLAGS += $(PLUGIN_LDFLAGS)
