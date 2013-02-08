/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A hook system.
 *
 */
#ifndef HOOK_H
#define HOOK_H

typedef struct hook_ hook_t;
typedef void (*hookfn_t)(void *data);

struct hook_ {
	stringref name;
	mowgli_list_t hooks;
};

E hook_t *hook_add_event(const char *);
E void hook_del_event(const char *);
E void hook_del_hook(const char *, hookfn_t);
E void hook_add_hook(const char *, hookfn_t);
E void hook_add_hook_first(const char *, hookfn_t);
E void hook_call_event(const char *, void *);

E void hook_stop(void);
E void hook_continue(void *newptr);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
