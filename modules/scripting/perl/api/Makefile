# Copyright (c) 2010 Atheme Development Group
# Rights to this code are documented in doc/LICENSE.

PLUGIN = Atheme$(PLUGIN_SUFFIX)

SRCS = Atheme.c perl_command.c

include ../../../../extra.mk
include ../../../../buildsys.mk

plugindir = $(MODDIR)/modules/scripting/lib

CPPFLAGS += -I../../../../include -I. -I../../../../libmowgli/src/libmowgli
CPPFLAGS += $(shell perl -MExtUtils::Embed -e ccopts) 
CFLAGS += $(PLUGIN_CFLAGS) -O0 -fno-inline-functions -ggdb3
LDFLAGS += $(PLUGIN_LDFLAGS)
LIBS += -L../../../../libathemecore -lathemecore ${LDFLAGS_RPATH} -L../../../../libmowgli/src/libmowgli

Atheme.c: $(wildcard *.xs) typemap
	xsubpp -prototypes -typemap typemap Atheme.xs >$@ || rm -f $@
