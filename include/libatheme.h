/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * libatheme stuff.
 *
 * $Id: libatheme.h 2679 2005-10-06 04:27:44Z nenolod $
 */

#ifndef LIBATHEME_H
#define LIBATHEME_H

#ifndef uint8_t
#define uint8_t u_int8_t
#define uint16_t u_int16_t
#define uint32_t u_int32_t
#define uint64_t u_int64_t
#endif

extern void (*clog)(uint32_t, const char *, ...);
extern void libclaro_init(void (*ilog)(uint32_t, const char *, ...));
extern void init_dlink_nodes(void);
extern void init_socket_queues(void);

#endif
