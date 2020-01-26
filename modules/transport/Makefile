# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.
# Copyright (C) 2010 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2011 Stephen Bennett <spb@exherbo.org>
# Copyright (C) 2014 Errietta Kostala <errietta@errietta.me>
# Copyright (C) 2018-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>

include ../../extra.mk

SUBDIRS = jsonrpc rfc1459 xmlrpc
MODULE  = transport
SRCS    = p10.c

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += -lathemecore
