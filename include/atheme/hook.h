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

struct hook_channel_joinpart
{
	/* Write NULL here if you kicked the user. When kicking the last user, you must join a service first,
	 * otherwise the channel may be destroyed and crashes may occur. The service may not part until you
	 * return; chanserv provides MC_INHABIT to help with this. This also prevents kick/rejoin floods. If
	 * this is NULL, a previous function kicked the user.
	 */
	struct chanuser *   cu;
};

struct hook_channel_message
{
	struct user *   u;              // Online user that sent the message
	struct channel *c;              // Channel the message was sent to
	char *          msg;            // The message itself
};

struct hook_channel_topic_check
{
	struct user *   u;              // Online user that changed the topic
	struct server * s;              // Server that restored a topic
	struct channel *c;              // Channel still has old topic
	const char *    setter;         // Stored setter string, can be nick, nick!user@host or server
	time_t          ts;             // Time the topic was changed
	const char *    topic;          // New topic
	int             approved;       // Write non-zero here to cancel the change
};

void hook_del_hook(const char *, hook_fn);
void hook_add_hook(const char *, hook_fn);
void hook_add_hook_first(const char *, hook_fn);
void hook_call_event(const char *, void *);

void hook_stop(void);
void hook_continue(void *newptr);

#endif /* !ATHEME_INC_HOOK_H */
