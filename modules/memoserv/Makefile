# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.

MODULE		= memoserv

SRCS =			\
	delete.c	\
	forward.c	\
	help.c		\
	ignore.c	\
	list.c		\
	main.c		\
	read.c		\
	send.c		\
	sendall.c	\
	sendgroup.c	\
	sendops.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LIBS		+= -L../../libathemecore -lathemecore
