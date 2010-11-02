# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
# $Id: Makefile.in 8375 2007-06-03 20:03:26Z pippijn $
#

MODULE = contrib

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

SRCS =			\
	cs_kickdots.c		\
	cs_ping.c		\
	cs_regnotice.c		\
	cs_sync.c		\
	cs_updown.c		\
	cs_userinfo.c		\
	gen_echoserver.c	\
	gen_httpd.c		\
	gen_listenerdemo.c	\
	gen_regcheckemail.c	\
	gen_vhostonreg.c	\
	graphtastical.c	\
	ircd_catserv.c	\
	ns_fenforce.c		\
	ns_fregister.c	\
	ns_forbid.c		\
	ns_generatehash.c	\
	ns_generatepass.c	\
	ns_guestnoreg.c	\
	ns_listlogins.c	\
	ns_mxcheck.c		\
	ns_mxcheck_async.c	\
	ns_regnotice.c	\
	ns_waitreg.c	\
	on_db_save.c		\
	os_akillnicklist.c	\
	os_kill.c		\
	os_klinechan.c	\
	os_pingspam.c		\
	os_savechanmodes.c	\
	os_tabletest.c	\
	os_testcmd.c		\
	os_testproc.c		\
	os_trace.c		\
	wumpus.c

CPPFLAGS	+= -I../../include
CFLAGS		+= ${PLUGIN_CFLAGS}
