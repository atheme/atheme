# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.

MODULE		= gameserv

SRCS = 			\
	dice.c		\
	eightball.c	\
	gamecalc.c	\
	help.c		\
	lottery.c	\
	main.c		\
	namegen.c	\
	rps.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LDFLAGS		+= -L../../libathemecore
LIBS		+= $(LIBMATH_LIBS) -lathemecore
