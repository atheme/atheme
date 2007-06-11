/**
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Memory functions.
 *
 * $Id: memory.c 8429 2007-06-10 18:51:23Z pippijn $
 */

#include "atheme.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

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

	strlcpy(t, s, len);
	return t;
}

/* stringbuffer operations */
void string_append(string_t *this, const char *src, size_t n)
{
	if (this->size - this->pos <= n)
	{
		int size = MAX(this->size * 2, this->pos + n + 8);
		char *new = (char *)realloc(this->str, size);
		this->size = size;
		this->str = new;
	}

	memcpy(this->str + this->pos, src, n);
	this->pos += n;
	this->str[this->pos] = 0;
}

void string_append_char(string_t *this, const char c)
{
	if (this->size - this->pos <= 1)
	{
		int size = MAX(this->size * 2, this->pos + 9);
		char *new = (char *)realloc(this->str, size);
		this->size = size;
		this->str = new;
	}

	this->str[this->pos++] = c;
	this->str[this->pos] = 0;
}

void string_reset(string_t *this)
{
	this->str[0] = this->pos = 0;
}

void string_delete(string_t *this)
{
	free (this->str);
	free (this);
}

string_t *new_string(size_t size)
{
	string_t *this = (string_t *)malloc(sizeof(string_t));

	this->size = size;
	this->pos  = 0;
	this->str  = (char *)malloc(size);

	/* Function pointers */
	this->append      = &string_append;
	this->append_char = &string_append_char;
	this->reset       = &string_reset;
	this->delete      = &string_delete;

	return this;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
