# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.

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
LDFLAGS		+= -L../../libathemecore
LIBS		+= -lathemecore ${LIBCRYPTO_LIBS}
