/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * libatheme stuff.
 *
 * $Id: libatheme.h 2885 2005-10-14 22:42:00Z nenolod $
 */

#ifndef LIBATHEME_H
#define LIBATHEME_H

extern void (*clog)(uint32_t, const char *, ...);
extern void libclaro_init(void (*ilog)(uint32_t, const char *, ...));
extern void init_dlink_nodes(void);
extern void init_socket_queues(void);

#endif
