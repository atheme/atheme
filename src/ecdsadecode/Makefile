PROG_NOINST	= ecdsadecode${PROG_SUFFIX}

SRCS = main.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= -I../../include
LIBS		+= -L../../libathemecore -lathemecore $(SSL_LIBS)
LDFLAGS		+= $(LDFLAGS_RPATH)

build: all
