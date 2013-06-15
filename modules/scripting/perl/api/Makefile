# Copyright (c) 2010 Atheme Development Group
# Rights to this code are documented in doc/LICENSE.

PLUGIN = Atheme$(PLUGIN_SUFFIX)

SRCS = Atheme.c perl_command.c perl_utilities.c perl_hooks.c

include ../../../../extra.mk
include ../../../../buildsys.mk

plugindir = $(MODDIR)/modules/scripting/lib

CPPFLAGS += -I../../../../include -I. -I../../../../libmowgli-2/src/libmowgli
CPPFLAGS += $(PERL_CFLAGS)
CFLAGS += $(PLUGIN_CFLAGS)
LDFLAGS += $(PLUGIN_LDFLAGS)
LIBS += -L../../../../libathemecore -lathemecore ${LDFLAGS_RPATH} -L../../../../libmowgli-2/src/libmowgli

# Some of the code from Perl is not designed to compile with warnings.
PERL_WARN_HACKS = -Wno-redundant-decls -Wno-float-equal -Wno-unused-value
PERL_WARN_HACKS += -Wno-nested-externs -Wno-unused-variable -Wno-shadow
CFLAGS += $(PERL_WARN_HACKS)
# This does not make any sense, but other warning flags are in CPPFLAGS too.
CPPFLAGS += $(PERL_WARN_HACKS)

Atheme.c: $(wildcard *.xs) typemap
	xsubpp -prototypes -typemap typemap Atheme.xs >$@ || rm -f $@

perl_hooks.c: make_perl_hooks.pl ../../../../include/hooktypes.in
	perl make_perl_hooks.pl
