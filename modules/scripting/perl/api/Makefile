# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2010 Atheme Project (http://atheme.org/)

PLUGIN		= Atheme$(PLUGIN_SUFFIX)
SRCS		= Atheme.c perl_command.c perl_hooks.c perl_utilities.c

include ../../../../extra.mk
include ../../../../buildsys.mk

plugindir	= $(MODDIR)/modules/scripting/lib

CPPFLAGS	+= -I../../../../include -I. $(PERL_CFLAGS)
LDFLAGS		+= -L../../../../libathemecore
LIBS		+= -lathemecore $(PERL_LIBS)

CFLAGS		+= $(PLUGIN_CFLAGS)
LDFLAGS		+= $(PLUGIN_LDFLAGS)

# Disable warnings on the Perl code; Perl's system headers are *garbage*
CFLAGS		+= -w

Atheme.c: $(wildcard *.xs) typemap
	xsubpp -prototypes -typemap typemap Atheme.xs >$@ || rm -f $@

perl_hooks.c: make_perl_hooks.pl ../../../../include/hooktypes.in
	perl make_perl_hooks.pl
