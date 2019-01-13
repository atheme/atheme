/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * Generic stuff from libathemecore.
 */

#include "sysconf.h"

#ifndef ATHEME_INC_LIBATHEMECORE_H
#define ATHEME_INC_LIBATHEMECORE_H 1

#include <stdbool.h>

#include "attrs.h"

bool atheme_thirdparty_libraries_early_init(void) ATHEME_FATTR_WUR;
void atheme_bootstrap(void);
void atheme_init(char *execname, char *log_p);
void atheme_setup(void);

int atheme_main(int argc, char *argv[]);

#endif /* !ATHEME_INC_LIBATHEMECORE_H */
