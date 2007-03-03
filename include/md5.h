/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Headers to MD5 hash functions
 * 
 * $Id: md5.h 7779 2007-03-03 13:55:42Z pippijn $
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
