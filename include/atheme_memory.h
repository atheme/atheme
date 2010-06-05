/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Memory stuff.
 *
 */

#ifndef __CLAROBASEMEMORY
#define __CLAROBASEMEMORY

E void *smalloc(size_t size);
E void *scalloc(size_t elsize, size_t els);
E void *srealloc(void *oldptr, size_t newsize);
E char *sstrdup(const char *s);
E char *sstrndup(const char *s, int len);

typedef struct string_ string_t;

struct string_
{
	char	*str;
	size_t	pos;
	size_t	size;

	void	(*reset)(struct string_ *this);
	void	(*append)(struct string_ *this, const char *src, size_t n);
	void	(*append_char)(struct string_ *this, const char c);
	void	(*sprintf)(struct string_ *this, const char *format, ...) PRINTFLIKE(2, 3);
	void	(*delete)(struct string_ *this);
};

/* stringbuffer operations */
E void string_append(string_t *this, const char *src, size_t n);
E void string_append_char(string_t *this, const char c);
E void string_reset(string_t *this);
E void string_delete(string_t *this);
E string_t *new_string(size_t size);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
