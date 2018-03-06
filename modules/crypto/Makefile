# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.

MODULE		= crypto

SRCS =				\
	argon2d.c		\
	blake2b.c		\
	crypt3-des.c		\
	crypt3-md5.c		\
	crypt3-sha2-256.c	\
	crypt3-sha2-512.c	\
	ircservices.c		\
	pbkdf2.c		\
	pbkdf2v2.c		\
	rawmd5.c		\
	rawsha1.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include -I.
LDFLAGS		+= -L../../libathemecore
LIBS 		+= -lathemecore $(LIBCRYPT_LIBS) $(LIBCRYPTO_LIBS) $(LIBIDN_LIBS)
