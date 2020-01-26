# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2010-2011 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2011 Jilles Tjoelker <jilles@stack.nl>
# Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2011 Nathan Phillip Brink <binki@gentoo.org>
# Copyright (C) 2012 JD Horelick <jdhore1@gmail.com>
# Copyright (C) 2018-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>

include ../../../extra.mk

plugindir = ${MODDIR}/modules/scripting
DISTCLEAN = perlxsi.c
SUBDIRS   = api
PLUGIN    = perl${PLUGIN_SUFFIX}
SRCS      =                 \
    perl_module.c           \
    perl_object_list.c      \
    perlxsi.c

include ../../../buildsys.mk

# Disable warnings on the Perl code; Perl's system headers are *garbage*
CFLAGS   += -w

LDFLAGS  += ${PLUGIN_LDFLAGS}
CFLAGS   += ${PLUGIN_CFLAGS}

CPPFLAGS += -I../../../include -I. -DPERL_MODDIR=\"${plugindir}\"
LDFLAGS  += -L../../../libathemecore
CFLAGS   += ${LIBPERL_CFLAGS}
LIBS     += ${LIBPERL_LIBS} -lathemecore

perlxsi.c:
	perl -MExtUtils::Embed -e xsinit

install-extra:
	${MKDIR_P} -- '${DESTDIR}'${plugindir}/examples
	${MKDIR_P} -- '${DESTDIR}'${plugindir}/lib
	${MKDIR_P} -- '${DESTDIR}'${plugindir}/lib/Atheme
	${MKDIR_P} -- '${DESTDIR}'${plugindir}/lib/Atheme/Internal
	${INSTALL} -m 644 examples/*.pl '${DESTDIR}'${plugindir}/examples
	${INSTALL} -m 644 lib/*.pl '${DESTDIR}'${plugindir}/lib
	${INSTALL} -m 644 lib/*.pm '${DESTDIR}'${plugindir}/lib
	${INSTALL} -m 644 lib/Atheme/*.pm '${DESTDIR}'${plugindir}/lib/Atheme
	${INSTALL} -m 644 lib/Atheme/Internal/*.pm '${DESTDIR}'${plugindir}/lib/Atheme/Internal
