# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.

MODULE		= crypto

SRCS =			\
	argon2d.c	\
	crypt3-des.c	\
	crypt3-md5.c	\
	ircservices.c	\
	pbkdf2.c	\
	pbkdf2v2.c	\
	rawmd5.c	\
	rawsha1.c

include ../../extra.mk
include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS	+= -I../../include -I.
LDFLAGS		+= -L../../libathemecore
LIBS 		+= -lathemecore ${LIBCRYPTO_LIBS} ${LIBIDN_LIBS}
