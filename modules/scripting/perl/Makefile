# Copyright (c) 2010 Atheme Development Group
# Rights to this code are documented in doc/LICENSE.

PLUGIN		= perl$(PLUGIN_SUFFIX)
SUBDIRS		= api
SRCS		= perl_module.c perl_object_list.c perlxsi.c

include ../../../extra.mk
include ../../../buildsys.mk

plugindir	= $(MODDIR)/modules/scripting

CPPFLAGS	+= -I../../../include -I. $(PERL_CFLAGS) -DPERL_MODDIR=\"$(plugindir)\"
LIBS		+= $(PERL_LIBS) -L../../../libathemecore -lathemecore

CFLAGS		+= $(PLUGIN_CFLAGS)
LDFLAGS		+= $(PLUGIN_LDFLAGS)

# Some of the code from Perl is not designed to compile with warnings.
CFLAGS		+= -Wno-redundant-decls -Wno-float-equal -Wno-unused-value
CFLAGS		+= -Wno-nested-externs -Wno-unused-variable -Wno-shadow

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
