# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#

MODULE = statserv

SRCS =			\
	main.c	\
	channel.c \
	server.c \
	netsplit.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LIBS +=	-L../../libathemecore -lathemecore ${LDFLAGS_RPATH}

