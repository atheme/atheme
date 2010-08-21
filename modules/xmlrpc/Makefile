PLUGIN = main$(PLUGIN_SUFFIX)

SRCS = main.c xmlrpclib.c

include ../../extra.mk
include ../../buildsys.mk

plugindir = $(MODDIR)/modules/xmlrpc

CPPFLAGS	+= -I../../include
CFLAGS		+= $(PLUGIN_CFLAGS)
LDFLAGS		+= $(PLUGIN_LDFLAGS)
