# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2003-2004 E. Will, et al.

MODULE		= crypto

SRCS =					\
	argon2.c			\
	crypt3-openbsd.c                \
	crypt3-sha2-256.c		\
	crypt3-sha2-512.c		\
	main.c                          \
	pbkdf2.c			\
	pbkdf2v2.c			\
	posix.c				\
	sodium-scrypt.c			\
	$(LEGACY_PWCRYPTO_COND_C)

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include -I.
LDFLAGS		+= -L../../libathemecore
CFLAGS		+= $(LIBARGON2_CFLAGS) $(LIBCRYPTO_CFLAGS) $(LIBIDN_CFLAGS) $(LIBSODIUM_CFLAGS)
LIBS 		+= $(LIBARGON2_LIBS) $(LIBCRYPTO_LIBS) $(LIBIDN_LIBS) $(LIBSODIUM_LIBS) $(LIBCRYPT_LIBS) -lathemecore
