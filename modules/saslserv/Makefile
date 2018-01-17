# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.

MODULE		= saslserv

SRCS =					\
	authcookie.c			\
	ecdsa-nist256p-challenge.c	\
	external.c			\
	main.c				\
	plain.c				\
	scram-sha.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include
LIBS		+= ${LIBCRYPTO_LIBS} -L../../libathemecore -lathemecore
