# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.

MODULE		= operserv

SRCS =				\
	akill.c			\
	clearchan.c		\
	clones.c		\
	compare.c		\
	genhash.c		\
	greplog.c		\
	help.c			\
	identify.c		\
	ignore.c		\
	info.c			\
	inject.c		\
	jupe.c			\
	main.c			\
	mode.c			\
	modinspect.c		\
	modlist.c		\
	modload.c		\
	modreload.c		\
	modunload.c		\
	noop.c			\
	raw.c			\
	rakill.c		\
	readonly.c		\
	rmatch.c		\
	rehash.c		\
	restart.c		\
	rnc.c			\
	rwatch.c		\
	set.c			\
	set_akicktime.c		\
	set_chanexpire.c	\
	set_commitinterval.c	\
	set_core.c		\
	set_enforceprefix.c	\
	set_klinetime.c		\
	set_maxchanacs.c	\
	set_maxchans.c		\
	set_maxfounders.c	\
	set_maxlogins.c		\
	set_maxnicks.c		\
	set_maxusers.c		\
	set_mdlimit.c		\
	set_nickexpire.c	\
	set_recontime.c		\
	set_spam.c		\
	shutdown.c		\
	sgline.c		\
	soper.c			\
	specs.c			\
	sqline.c		\
	uptime.c		\
	update.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LDFLAGS		+= -L../../libathemecore
LIBS		+= -lathemecore
