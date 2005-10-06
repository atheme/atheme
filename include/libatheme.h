/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * libatheme stuff.
 *
 * $Id: libatheme.h 2671 2005-10-06 04:03:49Z nenolod $
 */

#ifndef LIBATHEME_H
#define LIBATHEME_H

#ifndef uint8_t
typedef u_int8_t uint8_t;
typedef u_int16_t uint16_t;
typedef u_int32_t uint32_t;
typedef u_int64_t uint64_t;
#endif

extern void (*clog)(int, char *, ...);
extern void libclaro_init(void (*ilog)(uint32_t, const char *, ...));
extern void init_dlink_nodes(void);
extern void init_socket_queues(void);

#endif
