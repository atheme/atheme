PROG_NOINST	= ecdsadecode${PROG_SUFFIX}
SRCS		= main.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= -I../../include
LIBS		+= $(LIBCRYPTO_LIBS) -L../../libathemecore -lathemecore

build: all
