/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Config reader.
 */

#ifndef CONF_H
#define CONF_H

E bool conf_parse(const char *);
E void conf_init(void);
E bool conf_rehash(void);
E bool conf_check(void);

E void init_newconf(void);

/* XXX Unstable module api to add things to the standard conf blocks */
E mowgli_list_t conf_si_table; /* serverinfo{} */
E mowgli_list_t conf_ci_table; /* chanserv{} */
E mowgli_list_t conf_ni_table; /* nickserv{} */
E mowgli_list_t conf_gi_table; /* general{} */
E mowgli_list_t conf_la_table; /* language{} */

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
