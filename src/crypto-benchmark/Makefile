PROG		= $(PACKAGE)-crypto-benchmark${PROG_SUFFIX}
SRCS		= benchmark.c main.c optimal.c utils.c

include ../../extra.mk
include ../../buildsys.mk

CPPFLAGS	+= -I../../include
LDFLAGS		+= -L../../libathemecore
CFLAGS		+= $(LIBARGON2_CFLAGS)
LIBS		+= $(LIBARGON2_LIBS) $(LIBRT_LIBS) -lathemecore

build: all
