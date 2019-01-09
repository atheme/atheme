# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.

MODULE		= crypto

SRCS =					\
	argon2d.c			\
	blake2b.c			\
	crypt3-sha2-256.c		\
	crypt3-sha2-512.c		\
	pbkdf2.c			\
	pbkdf2v2.c			\
	posix.c				\
	$(LEGACY_PWCRYPTO_COND_C)

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include -I.
LDFLAGS		+= -L../../libathemecore
CFLAGS		+= $(LIBCRYPTO_CFLAGS) $(LIBIDN_CFLAGS)
LIBS 		+= $(LIBCRYPTO_LIBS) $(LIBIDN_LIBS) $(LIBCRYPT_LIBS) -lathemecore
