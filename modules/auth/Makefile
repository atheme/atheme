# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.

MODULE		= auth
SRCS		= ldap.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include $(LDAP_CPPFLAGS)
LDFLAGS		+= -L../../libathemecore $(LDAP_LDFLAGS)
LIBS 		+= -lathemecore $(LDAP_LIBS)
