# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.

MODULE		= groupserv

SRCS =				\
	acsnolimit.c		\
	drop.c			\
	fflags.c		\
	flags.c			\
	help.c			\
	info.c			\
	invite.c		\
	join.c			\
	list.c			\
	listchans.c		\
	main.c			\
	register.c		\
	regnolimit.c		\
	set_channel.c		\
	set_core.c		\
	set_description.c	\
	set_email.c		\
	set_groupname.c		\
	set_joinflags.c		\
	set_open.c		\
	set_public.c		\
	set_url.c		\

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LDFLAGS		+= -L../../libathemecore
LIBS		+= -lathemecore
