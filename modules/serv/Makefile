# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2026 Atheme Development Group (https://atheme.github.io/)
# Copyright (C) 2026 syk <syk@localhost>

include ../../extra.mk

MODULE = serv
SRCS   = \
    main.c

include ../../buildsys.mk
include ../../buildsys.module.mk

CPPFLAGS += -I../../include
LDFLAGS  += -L../../libathemecore
LIBS     += -lathemecore
