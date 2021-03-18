PROG		= dbverify${PROG_SUFFIX}
HELP_LINGUAS	= es ru

SRCS = main.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= -I../../include
LIBS		+= -L../../libathemecore -lathemecore
LDFLAGS		+= $(LDFLAGS_RPATH)

build: all
