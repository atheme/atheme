/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Callback management.
 *
 * $Id: callback.h 3001 2005-10-19 04:40:25Z nenolod $
 */

#ifndef CALLBACK_H
#define CALLBACK_H

struct callback_ {
	node_t node;
	char name[BUFSIZE];
	list_t hooks;

	/* Statistics */
	uint32_t calls;
};

typedef struct callback_ callback_t;

extern void        callback_init(void);
extern callback_t *callback_register(const char *name,
	void *(*func)(va_list args));
extern callback_t *callback_find(const char *name);
extern void *callback_execute(const char *name, ...);
extern void        callback_inject(const char *name,
	void *(*func)(va_list args));
extern void        callback_remove(const char *name,
	void *(*func)(va_list args));

#endif
