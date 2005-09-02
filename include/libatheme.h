/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * libatheme stuff.
 *
 * $Id: libatheme.h 1362 2005-08-01 07:03:43Z nenolod $
 */

#ifndef LIBATHEME_H
#define LIBATHEME_H

extern void libatheme_init(void (*ilog)(int, char *, ...));
extern void init_dlink_nodes(void);
extern void init_socket_queues(void);

#endif
