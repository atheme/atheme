# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
# $Id: Makefile.in 8375 2007-06-03 20:03:26Z pippijn $
#

MODULE = protocol

SRCS = \
	asuka.c	\
	bahamut.c	\
	base36uid.c	\
	charybdis.c	\
	elemental-ircd.c\
	hybrid.c	\
	inspircd.c	\
        ircd-seven.c  \
	ircnet.c	\
	mixin_nohalfops.c	\
	mixin_noholdnick.c	\
	mixin_noowner.c	\
	mixin_noprotect.c	\
	nefarious.c	\
	ngircd.c	\
	p10-generic.c	\
	ratbox.c	\
	shadowircd.c	\
	sporksircd.c	\
	ts6-generic.c	\
	undernet.c	\
	unreal.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LIBS +=	-L../../libathemecore -lathemecore ${LDFLAGS_RPATH}

