PROG		= dragon${PROG_SUFFIX}

SRCS = main.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= -I../../include
LIBS		+= -L../../libathemecore -lathemecore
LDFLAGS		+= $(LDFLAGS_RPATH)

build: all
