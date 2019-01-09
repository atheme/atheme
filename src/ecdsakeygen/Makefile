PROG		= ecdsakeygen${PROG_SUFFIX}
SRCS		= main.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= -I../../include
LDFLAGS		+= -L../../libathemecore
CFLAGS		+= $(LIBCRYPTO_CFLAGS) $(LIBSODIUM_CFLAGS)
LIBS		+= $(LIBCRYPTO_LIBS) $(LIBSODIUM_LIBS) -lathemecore

build: all
