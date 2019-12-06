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
