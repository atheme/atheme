PROG		= dragon${PROG_SUFFIX}

SRCS = main.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= $(MOWGLI_CFLAGS) $(PCRE_CFLAGS) -I../../include -DBINDIR=\"$(bindir)\"
LIBS		+= $(MOWGLI_LIBS) $(PCRE_LIBS) -L../../libathemecore -lathemecore
LDFLAGS		+= $(LDFLAGS_RPATH)

build: all
