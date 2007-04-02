/**
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Memory functions.
 *
 * $Id: memory.c 8079 2007-04-02 17:37:39Z nenolod $
 */

#include "atheme.h"

/* does malloc()'s job and dies if malloc() fails */
void *smalloc(size_t size)
{
        void *buf;

        buf = malloc(size);
        if (!buf)
                raise(SIGUSR1);
        return buf;
}

/* does calloc()'s job and dies if calloc() fails */
void *scalloc(size_t elsize, size_t els)
{
        void *buf = calloc(elsize, els);
        
        if (!buf)
                raise(SIGUSR1);
        return buf;
}

/* does realloc()'s job and dies if realloc() fails */
void *srealloc(void *oldptr, size_t newsize)
{
        void *buf = realloc(oldptr, newsize);

        if (!buf)
                raise(SIGUSR1);
        return buf;
}

/* does strdup()'s job, only with the above memory functions */
char *sstrdup(const char *s)
{
	char *t;

	if (!s || strlen(s) == 0)
		return NULL;

	t = smalloc(strlen(s) + 1);

	strcpy(t, s);
	return t;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
