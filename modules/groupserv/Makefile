
# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
# $Id: Makefile.in 8375 2007-06-03 20:03:26Z pippijn $
#

MODULE = groupserv

BASE_SRCS =			\
	acsnolimit.c		\
	drop.c			\
	fdrop.c			\
	fflags.c		\
	flags.c			\
	help.c			\
	info.c			\
	invite.c		\
	join.c			\
	list.c			\
	listchans.c		\
	register.c		\
	regnolimit.c		\
	set.c			\
	set_channel.c		\
	set_description.c	\
	set_email.c		\
	set_joinflags.c		\
	set_open.c		\
	set_public.c		\
	set_url.c		\

SRCS = ${BASE_SRCS} ${EXTRA_SRCS}

SUBDIRS = main

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LIBS +=	-L../../libathemecore -lathemecore ${LDFLAGS_RPATH}

