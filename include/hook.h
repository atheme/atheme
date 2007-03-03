/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A hook system.
 *
 * $Id: hook.h 7771 2007-03-03 12:46:36Z pippijn $
 */
#ifndef HOOK_H
#define HOOK_H

typedef struct hook_ hook_t;

struct hook_ {
	char *name;
	list_t hooks;
};

E void hook_add_event(const char *);
E void hook_del_event(const char *);
E void hook_del_hook(const char *, void (*)(void *));
E void hook_add_hook(const char *, void (*)(void *));
E void hook_add_hook_first(const char *, void (*)(void *));
E void hook_call_event(const char *, void *);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
