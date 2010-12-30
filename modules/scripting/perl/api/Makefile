# Copyright (c) 2010 Atheme Development Group
# Rights to this code are documented in doc/LICENSE.

PLUGIN = Atheme$(PLUGIN_SUFFIX)

SRCS = Atheme.c perl_command.c

include ../../../../extra.mk
include ../../../../buildsys.mk

plugindir = $(MODDIR)/modules/scripting/lib

CPPFLAGS += -I../../../../include -I. -I../../../../libmowgli/src/libmowgli
CPPFLAGS += $(PERL_CFLAGS)
CFLAGS += $(PLUGIN_CFLAGS)
LDFLAGS += $(PLUGIN_LDFLAGS)
LIBS += -L../../../../libathemecore -lathemecore ${LDFLAGS_RPATH} -L../../../../libmowgli/src/libmowgli

Atheme.c: $(wildcard *.xs) typemap
	xsubpp -prototypes -typemap typemap Atheme.xs >$@ || rm -f $@
