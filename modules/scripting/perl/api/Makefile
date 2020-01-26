# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2010-2013 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2011 Jilles Tjoelker <jilles@stack.nl>
# Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2018-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>

include ../../../../extra.mk

plugindir = ${MODDIR}/modules/scripting/lib
DISTCLEAN = Atheme.c perl_hooks.c
PLUGIN    = Atheme${PLUGIN_SUFFIX}
SRCS      = Atheme.c perl_command.c perl_hooks.c perl_utilities.c

include ../../../../buildsys.mk

# Disable warnings on the Perl code; Perl's system headers are *garbage*
CFLAGS   += -w

LDFLAGS  += ${PLUGIN_LDFLAGS}
CFLAGS   += ${PLUGIN_CFLAGS}

CPPFLAGS += -I../../../../include -I.
LDFLAGS  += -L../../../../libathemecore
CFLAGS   += ${LIBPERL_CFLAGS}
LIBS     += ${LIBPERL_LIBS} -lathemecore

Atheme.c: $(wildcard *.xs) typemap
	@xsubpp -prototypes -typemap typemap Atheme.xs >$@ || rm -f $@

perl_hooks.c: make_perl_hooks.pl ../../../../include/atheme/hooktypes.in
	@perl make_perl_hooks.pl
