# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.

MODULE		= gameserv

SRCS = 			\
	dice.c		\
	eightball.c	\
	gamecalc.c	\
	happyfarm.c	\
	help.c		\
	lottery.c	\
	main.c		\
	namegen.c	\
	rps.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LIBS		+= -L../../libathemecore -lathemecore -lm
