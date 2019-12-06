/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * A hook system.
 */

#ifndef ATHEME_INC_HOOK_H
#define ATHEME_INC_HOOK_H 1

#include <atheme/common.h>
#include <atheme/stdheaders.h>
#include <atheme/structures.h>

// Types necessary for the hook system (and/or for Perl scripts)
typedef struct myentity myentity_t;
typedef struct mygroup mygroup_t;
typedef struct mynick mynick_t;
typedef struct myuser myuser_t;
typedef struct sasl_message sasl_message_t;
typedef struct server server_t;
typedef struct service service_t;
typedef struct sourceinfo sourceinfo_t;
typedef struct user user_t;



typedef void (*hook_fn)(void *data);

struct hook
{
	stringref       name;
	mowgli_list_t   hooks;
};

void hook_del_hook(const char *, hook_fn);
void hook_add_hook(const char *, hook_fn);
void hook_add_hook_first(const char *, hook_fn);
void hook_call_event(const char *, void *);

void hook_stop(void);
void hook_continue(void *newptr);

#endif /* !ATHEME_INC_HOOK_H */
