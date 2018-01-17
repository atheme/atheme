PROG		= ecdsakeygen${PROG_SUFFIX}

SRCS = main.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= -I../../include
LIBS		+= -L../../libathemecore -lathemecore $(LIBCRYPTO_LIBS)

build: all
