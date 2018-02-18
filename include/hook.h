/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A hook system.
 */

#ifndef HOOK_H
#define HOOK_H

// Types necessary for the hook system
typedef struct chanacs chanacs_t;
typedef struct channel channel_t;
typedef struct chanuser chanuser_t;
typedef struct database_handle database_handle_t;
typedef struct myentity myentity_t;
typedef struct sasl_message sasl_message_t;
typedef struct service service_t;
typedef struct sourceinfo sourceinfo_t;



typedef void (*hook_fn)(void *data);

struct hook
{
	stringref name;
	mowgli_list_t hooks;
};

extern struct hook *hook_add_event(const char *);
extern void hook_del_event(const char *);
extern void hook_del_hook(const char *, hook_fn);
extern void hook_add_hook(const char *, hook_fn);
extern void hook_add_hook_first(const char *, hook_fn);
extern void hook_call_event(const char *, void *);

extern void hook_stop(void);
extern void hook_continue(void *newptr);

#endif
