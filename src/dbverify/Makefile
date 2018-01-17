PROG		= dbverify${PROG_SUFFIX}
HELP_LINGUAS	= es ru

SRCS = main.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= $(PCRE_CFLAGS) -I../../include
LIBS		+= $(PCRE_LIBS) -L../../libathemecore -lathemecore

build: all
