# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.

MODULE		= auth
SRCS		= dummy.c $(LDAP_AUTH_COND_C)

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include $(LDAP_CFLAGS)
LDFLAGS		+= -L../../libathemecore
LIBS 		+= -lathemecore $(LDAP_LIBS)
