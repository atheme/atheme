/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Generic stuff from libathemecore.
 */

#ifndef LIBATHEMECORE_H
#define LIBATHEMECORE_H

extern void atheme_bootstrap(void);
extern void atheme_init(char *execname, char *log_p);
extern void atheme_setup(void);

extern int atheme_main(int argc, char *argv[]);

#endif
