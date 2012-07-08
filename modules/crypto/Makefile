# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
# $Id: Makefile.in 8375 2007-06-03 20:03:26Z pippijn $
#

MODULE = crypto

SRCS =			\
	pbkdf2.c	\
	posix.c		\
	ircservices.c		\
	rawmd5.c		\
	rawsha1.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
# Only rawsha1.so needs this, oh well.
LIBS		+= ${SSL_LIBS}
LIBS 		+= -L../../libathemecore -lathemecore ${LDFLAGS_RPATH}

