/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Generic stuff from libathemecore.
 */

#ifndef LIBATHEMECORE_H
#define LIBATHEMECORE_H

void atheme_bootstrap(void);
void atheme_init(char *execname, char *log_p);
void atheme_setup(void);

int atheme_main(int argc, char *argv[]);

#endif
