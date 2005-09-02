/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A hook system.
 *
 * $Id: hook.h 841 2005-07-16 07:59:13Z nenolod $
 */
#ifndef HOOK_H
#define HOOK_H

typedef struct hook_ hook_t;

struct hook_ {
	char *name;
	list_t hooks;
};

typedef struct {
	channel_t *c;
	char *msg;
} hook_cmessage_data_t;

extern void hooks_init(void);
extern void hook_add_event(const char *);
extern void hook_del_event(const char *);
extern void hook_del_hook(const char *, void (*)(void *));
extern void hook_add_hook(const char *, void (*)(void *));
extern void hook_call_event(const char *, void *);

#endif
