# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.

MODULE		= transport
SUBDIRS		= xmlrpc rfc1459 jsonrpc
SRCS		= p10.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS        += -I../../include
LIBS		+= -L../../libathemecore -lathemecore
