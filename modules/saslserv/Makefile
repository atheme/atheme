# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.

MODULE		= saslserv

SRCS =					\
	authcookie.c			\
	ecdh-x25519-challenge.c		\
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
CFLAGS		+= $(LIBCRYPTO_CFLAGS) $(LIBNETTLE_CFLAGS) $(LIBSODIUM_CFLAGS)
LIBS		+= $(LIBCRYPTO_LIBS) $(LIBMBEDCRYPTO_LIBS) $(LIBNETTLE_LIBS) $(LIBSODIUM_LIBS) -lathemecore
