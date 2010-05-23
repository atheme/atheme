# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
# $Id: Makefile.in 8375 2007-06-03 20:03:26Z pippijn $
#

MODULE = auth

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

SRCS =			\
	dummy.c				\
	$(LDAP_AUTHC)

CPPFLAGS	+= -I../../include $(LDAP_CFLAGS)
LIBS		+= $(LDAP_LIBS)
