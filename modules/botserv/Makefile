# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.
# Copyright (C) 2016 Austin Ellis

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
LDFLAGS		+= -L../../libathemecore
LIBS		+= -lathemecore
