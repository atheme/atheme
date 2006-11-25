/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * stuff for internal use by libatheme.
 *
 * $Id: claro_internal.h 7273 2006-11-25 00:25:20Z jilles $
 */

/* log function pointer */
E void (*claro_log)(uint32_t, const char *, ...);

/* internal functions */
E void event_init(void);
E void hooks_init(void);
E void init_dlink_nodes(void);
E void init_netio(void);
E void init_socket_queues(void);
