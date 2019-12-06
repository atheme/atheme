# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2010 Atheme Project (http://atheme.org/)

PLUGIN		= Atheme$(PLUGIN_SUFFIX)
SRCS		= Atheme.c perl_command.c perl_hooks.c perl_utilities.c

DISTCLEAN	= Atheme.c perl_hooks.c

include ../../../../extra.mk
include ../../../../buildsys.mk

plugindir	= $(MODDIR)/modules/scripting/lib

CPPFLAGS	+= -I../../../../include -I.
LDFLAGS		+= -L../../../../libathemecore
CFLAGS		+= $(LIBPERL_CFLAGS)
LIBS		+= $(LIBPERL_LIBS) -lathemecore

LDFLAGS		+= $(PLUGIN_LDFLAGS)
CFLAGS		+= $(PLUGIN_CFLAGS)

# Disable warnings on the Perl code; Perl's system headers are *garbage*
CFLAGS		+= -w

Atheme.c: $(wildcard *.xs) typemap
	xsubpp -prototypes -typemap typemap Atheme.xs >$@ || rm -f $@

perl_hooks.c: make_perl_hooks.pl ../../../../include/atheme/hooktypes.in
	perl make_perl_hooks.pl
