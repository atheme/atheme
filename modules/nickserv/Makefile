# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
# $Id: Makefile.in 8375 2007-06-03 20:03:26Z pippijn $
#

MODULE = nickserv

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

BASE_SRCS =			\
	main.c	\
	access.c	\
	cert.c	\
	drop.c	\
	enforce.c	\
	freeze.c	\
	ghost.c	\
	group.c	\
	help.c	\
	hold.c	\
	identify.c	\
	info.c	\
	list.c	\
	listchans.c	\
	listmail.c	\
	listownmail.c	\
	login.c		\
	logout.c	\
	mark.c	\
	register.c	\
	resetpass.c	\
	return.c	\
	sendpass.c	\
	sendpass_user.c \
	set.c		\
	set_accountname.c \
	set_email.c	\
	set_emailmemos.c  \
	set_hidemail.c	\
	set_neverop.c	\
	set_nomemo.c	\
	set_privmsg.c	\
	set_private.c	\
	set_quietchg.c	\
	setpass.c	\
	status.c	\
	subscribe.c	\
	taxonomy.c	\
	vacation.c	\
	verify.c	\
	vhost.c

SRCS = ${BASE_SRCS} ${EXTRA_SRCS}

CPPFLAGS	+= -I../../include
