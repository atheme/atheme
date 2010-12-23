# Copyright (c) 2010 Atheme Development Group
# Rights to this code are documented in doc/LICENSE.

PLUGIN = main$(PLUGIN_SUFFIX)

SRCS = main.c gather.c fix.c

include ../../extra.mk
include ../../buildsys.mk

plugindir = $(MODDIR)/modules/chanfix

CPPFLAGS += -I../../include
CFLAGS += $(PLUGIN_CFLAGS)
LDFLAGS += $(PLUGIN_LDFLAGS)
LIBS +=	-L../../libathemecore -lathemecore ${LDFLAGS_RPATH}

