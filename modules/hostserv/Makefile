# Copyright (c) 2015-2016 Atheme Development Group <http://atheme.github.io>
# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
# $Id: Makefile.in 8375 2007-06-03 20:03:26Z pippijn $
#

MODULE = hostserv

SRCS = drop.c  group.c  help.c  main.c  offer.c  onoff.c  request.c  vhost.c  vhostnick.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LIBS +=	-L../../libathemecore -lathemecore ${LDFLAGS_RPATH}

