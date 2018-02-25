/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Generic stuff from libathemecore.
 */

#ifndef ATHEME_INC_LIBATHEMECORE_H
#define ATHEME_INC_LIBATHEMECORE_H 1

void atheme_bootstrap(void);
void atheme_init(char *execname, char *log_p);
void atheme_setup(void);

int atheme_main(int argc, char *argv[]);

#endif /* !ATHEME_INC_LIBATHEMECORE_H */
