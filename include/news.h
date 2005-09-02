/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for news information.
 *
 * $Id: news.h 190 2005-05-29 18:48:17Z nenolod $
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
