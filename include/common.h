/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for account information.
 *
 * $Id: common.h 2671 2005-10-06 04:03:49Z nenolod $
 */

#ifndef COMMON_H
#define COMMON_H

/* D E F I N E S */
#define BUFSIZE  1024            /* maximum size of a buffer */
#define MODESTACK_WAIT 500
#define MAXMODES 4
#define MAXPARAMSLEN (510-32-64-34-(7+MAXMODES))
#define MAX_EVENTS 25
#define TOKEN_UNMATCHED -1
#define TOKEN_ERROR -2

/* lengths of buffers (string length is 1 less) */
#define NICKLEN 50
#define USERLEN 11
#define HOSTLEN 64
#define GECOSLEN 51
#define EMAILLEN 120
#define MEMOLEN 129

/* H A S H */
#define HASHINIT 0x811c9dc5
#define HASHBITS 16
#define HASHSIZE (1 << HASHBITS)  /* 2^16 = 65536 */

#undef DEBUG_BALLOC

#ifdef DEBUG_BALLOC
#define BALLOC_MAGIC 0x3d3a3c3d
#endif

#ifdef LARGE_NETWORK
#define HEAP_CHANNEL    1024
#define HEAP_CHANUSER   1024
#define HEAP_USER       1024
#define HEAP_SERVER     16
#define HEAP_NODE       1024  
#define HEAP_CHANACS    1024
#else
#define HEAP_CHANNEL    64
#define HEAP_CHANUSER   128
#define HEAP_USER       128
#define HEAP_SERVER     8
#define HEAP_NODE       128
#define HEAP_CHANACS    128
#endif

#define CACHEFILE_HEAP_SIZE     32
#define CACHELINE_HEAP_SIZE     64

#endif
