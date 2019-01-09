PROG_NOINST	= b64test${PROG_SUFFIX}
SRCS		= main.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= -I../../include
LDFLAGS		+= -L../../libathemecore
CFLAGS		+= $(LIBSODIUM_CFLAGS)
LIBS		+= $(LIBSODIUM_LIBS) -lathemecore

build: all
