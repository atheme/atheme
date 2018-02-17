/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Config reader.
 */

#ifndef CONF_H
#define CONF_H

extern bool conf_parse(const char *);
extern void conf_init(void);
extern bool conf_rehash(void);
extern bool conf_check(void);

extern void init_newconf(void);

/* XXX Unstable module api to add things to the standard conf blocks */
extern mowgli_list_t conf_si_table; /* serverinfo{} */
extern mowgli_list_t conf_gi_table; /* general{} */
extern mowgli_list_t conf_la_table; /* language{} */

#endif
