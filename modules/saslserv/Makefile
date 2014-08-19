# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
# $Id: Makefile.in 8375 2007-06-03 20:03:26Z pippijn $
#

MODULE = saslserv

SRCS = \
	authcookie.c	\
	ecdsa-nist256p-challenge.c \
	external.c	\
	main.c		\
	plain.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LIBS		+= ${SSL_LIBS}
LIBS		+= -L../../libathemecore -lathemecore ${LDFLAGS_RPATH}

