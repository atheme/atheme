PROG_NOINST	= dragon${PROG_SUFFIX}
SRCS		= main.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= $(PCRE_CFLAGS) -I../../include
LIBS		+= $(PCRE_LIBS) -L../../libathemecore -lathemecore

build: all
