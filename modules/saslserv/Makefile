# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
# $Id: Makefile.in 8375 2007-06-03 20:03:26Z pippijn $
#

MODULE = saslserv

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

SRCS = \
	authcookie.c	\
	crypt.c		\
	dh-blowfish.c	\
	main.c		\
	plain.c

CPPFLAGS	+= -I../../include
# Only dh-blowfish.so needs this, oh well.
LIBS		+= ${SSL_LIBS}
