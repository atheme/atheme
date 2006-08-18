/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Defines needed by multiple header files.
 *
 * $Id: common.h 6103 2006-08-18 00:04:43Z jilles $
 */

#ifndef COMMON_H
#define COMMON_H

/* D E F I N E S */
#define BUFSIZE 1024            /* maximum size of a buffer */
#define MAXMODES 4

/* lengths of buffers (string length is 1 less) */
#define NICKLEN 51
#define IDLEN 10
#define CHANNELLEN 201
#define USERLEN 11
#define HOSTIPLEN 54
#define GECOSLEN 51
#define KEYLEN 24
#define EMAILLEN 120
#define MEMOLEN 300

#define MAXMSIGNORES 40

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
#define HEAP_CHANACS    1024
#else
#define HEAP_CHANNEL    64
#define HEAP_CHANUSER   128
#define HEAP_USER       128
#define HEAP_SERVER     8
#define HEAP_CHANACS    128
#endif

#define CACHEFILE_HEAP_SIZE     32
#define CACHELINE_HEAP_SIZE     64

#endif
