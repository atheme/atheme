/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Stuff for internal use in Atheme.
 *
 * $Id: internal.h 7467 2007-01-14 03:25:42Z nenolod $
 */

/* log function pointer */
E void (*claro_log)(uint32_t, const char *, ...);

/* internal functions */
E void event_init(void);
E void hooks_init(void);
E void init_dlink_nodes(void);
E void init_netio(void);
E void init_socket_queues(void);
