/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_hash.c: FNV-1 hashing implementation.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice is present in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "mowgli.h"

#define HASHINIT 0x811c9dc5
#define HASHBITS 16
#define HASHSIZE (1 << HASHBITS)  /* 2^16 = 65536 */

int mowgli_fnv_hash_string(const char *p)
{
	static int htoast = 0;
        unsigned int hval = HASHINIT;

	if (htoast == 0)
		htoast = rand();

        if (!p)
                return (0);
        for (; *p != '\0'; ++p)
        {
                hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
                hval ^= (tolower(*p) ^ htoast);
        }

        return ((hval >> HASHBITS) ^ (hval & ((1 << HASHBITS) - 1)) % HASHSIZE);
}

int mowgli_fnv_hash(unsigned int *p)
{
	static int htoast = 0;
        unsigned int hval = HASHINIT;

	if (htoast == 0)
		htoast = rand();

        if (!p)
                return (0);
        for (; *p != '\0'; ++p)
        {
                hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
                hval ^= (tolower(*p) ^ htoast);
        }

        return ((hval >> HASHBITS) ^ (hval & ((1 << HASHBITS) - 1)) % HASHSIZE);
}
