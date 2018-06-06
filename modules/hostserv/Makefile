# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.
# Copyright (C) 2015-2016 Atheme Development Group <http://atheme.github.io>

MODULE		= hostserv

SRCS = 			\
	drop.c		\
	group.c		\
	help.c		\
	main.c		\
	offer.c		\
	onoff.c		\
	request.c	\
	vhost.c		\
	vhostnick.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LDFLAGS		+= -L../../libathemecore
LIBS		+= -lathemecore
