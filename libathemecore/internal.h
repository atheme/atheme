/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Stuff for internal use in Atheme.
 *
 */

#ifndef INTERNAL_H
#define INTERNAL_H

/* internal functions */
E void event_init(void);
E void hooks_init(void);
E void init_dlink_nodes(void);
E void init_netio(void);
E void init_socket_queues(void);
E void init_signal_handlers(void);

E void language_init(void);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
