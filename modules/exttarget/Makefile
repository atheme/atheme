# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.

MODULE		= exttarget
SRCS		= chanacs.c channel.c main.c oper.c registered.c server.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LIBS		+= -L../../libathemecore -lathemecore
