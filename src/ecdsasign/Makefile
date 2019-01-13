PROG		= ecdsasign${PROG_SUFFIX}
SRCS		= main.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= -I../../include
LDFLAGS		+= -L../../libathemecore
CFLAGS		+= $(LIBCRYPTO_CFLAGS)
LIBS		+= $(LIBCRYPTO_LIBS) -lathemecore

build: all
