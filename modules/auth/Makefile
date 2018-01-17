# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.

MODULE		= auth
SRCS		= dummy.c $(LDAP_AUTHC)

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= $(LDAP_CFLAGS) -I../../include
LIBS 		+= $(LDAP_LIBS) -L../../libathemecore -lathemecore
