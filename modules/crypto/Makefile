# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
# $Id: Makefile.in 8375 2007-06-03 20:03:26Z pippijn $
#

MODULE = crypto

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

SRCS =			\
	$(POSIX_CRYPTC)	\
	ircservices.c		\
	rawmd5.c		\
	rawsha1.c		\
	sorservices.c

CPPFLAGS	+= -I../../include
# Only rawsha1.so needs this, oh well.
LIBS		+= ${SSL_LIBS}
