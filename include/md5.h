/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Headers to MD5 hash functions
 * 
 * $Id: md5.h 5107 2006-04-17 17:48:00Z gxti $
 */

#ifndef MD5_H
#define MD5_H

typedef unsigned int UINT4;

/* MD5 context. */
typedef struct {
    UINT4 state[4];             /* state (ABCD) */
    UINT4 count[2];             /* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64];   /* input buffer */
} MD5_CTX;

void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context, unsigned const char *input, unsigned int inputLen);
void MD5Final(unsigned char digest[16], MD5_CTX *context);

#endif
