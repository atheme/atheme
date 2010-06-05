/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for news information.
 *
 */

#ifndef NEWS_H
#define NEWS_H

typedef struct news_ news_t;

struct news_ {
	myuser_t *user;
	time_t    date;
	char	 *text;
};

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
