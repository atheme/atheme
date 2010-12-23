PLUGIN = rfc1459$(PLUGIN_SUFFIX)

SRCS = main.c parse.c

include ../../../extra.mk
include ../../../buildsys.mk

plugindir = $(MODDIR)/modules/transport

CPPFLAGS	+= -I../../../include
CFLAGS		+= $(PLUGIN_CFLAGS)
LDFLAGS		+= $(PLUGIN_LDFLAGS)
LIBS +=	-L../../../libathemecore -lathemecore ${LDFLAGS_RPATH}

