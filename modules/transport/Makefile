# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.

MODULE		= transport
SUBDIRS		= xmlrpc rfc1459 jsonrpc
SRCS		= p10.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS        += -I../../include
LDFLAGS		+= -L../../libathemecore
LIBS		+= -lathemecore
