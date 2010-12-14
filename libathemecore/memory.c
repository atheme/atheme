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

/* does malloc()'s job and dies if malloc() fails */
void *smalloc(size_t size)
{
        void *buf = calloc(size, 1);

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

	strlcpy(t, s, len + 1);
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
	string_t *this = (string_t *)smalloc(sizeof(string_t));

	this->size = size;
	this->pos  = 0;
	this->str  = (char *)smalloc(size);

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
