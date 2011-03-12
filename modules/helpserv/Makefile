# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
#

MODULE = helpserv

SRCS = main.c helpme.c services.c ticket.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LIBS +=	-L../../libathemecore -lathemecore ${LDFLAGS_RPATH}

