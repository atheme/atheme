# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2010 Atheme Project (http://atheme.org/)

PLUGIN		= perl$(PLUGIN_SUFFIX)
SUBDIRS		= api
SRCS		= perl_module.c perl_object_list.c perlxsi.c

DISTCLEAN	= perlxsi.c

include ../../../extra.mk
include ../../../buildsys.mk

plugindir	= $(MODDIR)/modules/scripting

CPPFLAGS	+= -I../../../include -I. -DPERL_MODDIR=\"$(plugindir)\"
LDFLAGS		+= -L../../../libathemecore
CFLAGS		+= $(LIBPERL_CFLAGS)
LIBS		+= $(LIBPERL_LIBS) -lathemecore

LDFLAGS		+= $(PLUGIN_LDFLAGS)
CFLAGS		+= $(PLUGIN_CFLAGS)

# Disable warnings on the Perl code; Perl's system headers are *garbage*
CFLAGS		+= -w

perlxsi.c:
	perl -MExtUtils::Embed -e xsinit

install-extra:
	${MKDIR_P} '${DESTDIR}'${plugindir}/lib
	${INSTALL} -m644 lib/*.pl lib/*.pm '${DESTDIR}'${plugindir}/lib
	${MKDIR_P} '${DESTDIR}'${plugindir}/lib/Atheme
	${INSTALL} -m644 lib/Atheme/*.pm '${DESTDIR}'${plugindir}/lib/Atheme
	${MKDIR_P} '${DESTDIR}'${plugindir}/lib/Atheme/Internal
	${INSTALL} -m644 lib/Atheme/Internal/*.pm '${DESTDIR}'${plugindir}/lib/Atheme/Internal
	${MKDIR_P} '${DESTDIR}'${plugindir}/examples
	${INSTALL} -m644 examples/*.pl '${DESTDIR}'${plugindir}/examples
