/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Stuff for internal use in Atheme.
 */

#ifndef ATHEME_LAC_INTERNAL_H
#define ATHEME_LAC_INTERNAL_H 1

/* internal functions */
void event_init(void);
void hooks_init(void);
void init_dlink_nodes(void);
void init_netio(void);
void init_socket_queues(void);
void init_signal_handlers(void);

void language_init(void);

#endif /* !ATHEME_LAC_INTERNAL_H */
