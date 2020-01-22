PROG_NOINST	= $(PACKAGE)-b64test${PROG_SUFFIX}
SRCS		= main.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= -I../../include
LDFLAGS		+= -L../../libathemecore
LIBS		+= $(LIBINTL) -lathemecore

build: all
