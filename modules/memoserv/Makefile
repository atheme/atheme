# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.

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
LDFLAGS		+= -L../../libathemecore
LIBS		+= -lathemecore
