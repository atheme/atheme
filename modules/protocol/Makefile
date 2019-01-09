# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.

MODULE		= protocol

SRCS =				\
	asuka.c			\
	bahamut.c		\
	base36uid.c		\
	charybdis.c		\
	chatircd1.1.c		\
	elemental-ircd.c	\
	inspircd.c		\
	ircd-seven.c		\
	ircnet.c		\
	mixin_nohalfops.c	\
	mixin_noholdnick.c	\
	mixin_noowner.c		\
	mixin_noprotect.c	\
	nefarious.c		\
	ngircd.c		\
	p10-generic.c		\
	ratbox.c		\
	ts6-generic.c		\
	unreal.c		\
	unreal4.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LDFLAGS		+= -L../../libathemecore
LIBS		+= -lathemecore
