# Copyright (c) 2016 Austin Ellis
# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.

MODULE		= botserv

SRCS =			\
	bottalk.c	\
	help.c		\
	info.c		\
	main.c		\
	set.c		\
	set_core.c	\
	set_fantasy.c	\
	set_nobot.c	\
	set_private.c	\
	set_saycaller.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LIBS		+= -L../../libathemecore -lathemecore
