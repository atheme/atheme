/*
 * atheme-services: A collection of minimalist IRC services
 * memory.c: Logging routines
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
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

#include "atheme.h"

#ifndef SIGUSR1
# define RAISE_EXCEPTION abort()
#else
# define RAISE_EXCEPTION raise(SIGUSR1)
#endif

/* does malloc()'s job and dies if malloc() fails */
void *smalloc(size_t size)
{
        void *buf = calloc(size, 1);

        if (!buf)
                RAISE_EXCEPTION;
        return buf;
}

/* does calloc()'s job and dies if calloc() fails */
void *scalloc(size_t elsize, size_t els)
{
        void *buf = calloc(elsize, els);

        if (!buf)
                RAISE_EXCEPTION;
        return buf;
}

/* does realloc()'s job and dies if realloc() fails */
void *srealloc(void *oldptr, size_t newsize)
{
        void *buf = realloc(oldptr, newsize);

        if (!buf)
                RAISE_EXCEPTION;
        return buf;
}

/* does strdup()'s job, only with the above memory functions */
char *sstrdup(const char *s)
{
	char *t;

	if (s == NULL)
		return NULL;

	t = smalloc(strlen(s) + 1);

	strcpy(t, s);
	return t;
}

/* does strndup()'s job, only with the above memory functions */
char *sstrndup(const char *s, int len)
{
	char *t;

	if (s == NULL)
		return NULL;

	t = smalloc(len + 1);

	mowgli_strlcpy(t, s, len + 1);
	return t;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
