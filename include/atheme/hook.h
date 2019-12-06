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

struct hook_channel_acl_req
{
	struct chanacs *    ca;
	struct sourceinfo * si;
	struct myentity *   parent;
	unsigned int        oldlevel;
	unsigned int        newlevel;
	int                 approved;
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

struct hook_channel_mode
{
	struct user *   u;
	struct channel *c;
};

struct hook_channel_mode_change
{
	struct chanuser *   cu;
	const char          mchar;
	const unsigned int  mvalue;
};

struct hook_channel_register_check
{
	struct sourceinfo * si;
	const char *        name;
	struct channel *    chan;
	int                 approved;   // Write non-zero here to disallow the registration
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

struct hook_server_delete
{
	struct server * s;
	/* space for reason etc here */
};

struct hook_user_delete_info
{
	struct user * const u;
	const char *        comment;
};

struct hook_user_nick
{
	struct user *    u;             // User in question. Write NULL here if you delete the user
	const char *     oldnick;       // Previous nick for nick changes. u->nick is the new nick
};

void hook_del_hook(const char *, hook_fn);
void hook_add_hook(const char *, hook_fn);
void hook_add_hook_first(const char *, hook_fn);
void hook_call_event(const char *, void *);

void hook_stop(void);
void hook_continue(void *newptr);

#endif /* !ATHEME_INC_HOOK_H */
