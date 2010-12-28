
# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
# $Id: Makefile.in 8375 2007-06-03 20:03:26Z pippijn $
#

MODULE = groupserv

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

SUBDIRS = main

BASE_SRCS =			\
	acsnolimit.c		\
	drop.c			\
	fdrop.c			\
	fflags.c		\
	flags.c			\
	help.c			\
	info.c			\
	join.c			\
	list.c			\
	register.c		\
	regnolimit.c		\

SRCS = ${BASE_SRCS} ${EXTRA_SRCS}

CPPFLAGS	+= -I../../include
LIBS +=	-L../../libathemecore -lathemecore ${LDFLAGS_RPATH}

