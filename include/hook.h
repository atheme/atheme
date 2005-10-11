/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A hook system.
 *
 * $Id: hook.h 2835 2005-10-11 05:27:26Z terminal $
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

E void hooks_init(void);
E void hook_add_event(const char *);
E void hook_del_event(const char *);
E void hook_del_hook(const char *, void (*)(void *));
E void hook_add_hook(const char *, void (*)(void *));
E void hook_call_event(const char *, void *);

#endif
