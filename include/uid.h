/*
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * UID provider stuff.
 */

#ifndef __UID_H__
#define __UID_H__

typedef struct {
	void (*uid_init)(const char *sid);
	const char *(*uid_get)(void);
} uid_provider_t;

extern uid_provider_t *__uid_provider;

#endif
