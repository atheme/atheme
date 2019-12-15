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

struct hook_channel_req
{
	struct mychan *     mc;
	struct sourceinfo * si;
};

struct hook_channel_succession_req
{
	struct mychan * mc;
	struct myuser * mu;
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

struct hook_chanuser_sync
{
	struct chanuser *  cu;            // Write NULL if you kicked the user. You must not destroy the channel.
	const unsigned int flags;         // The user's channel access, if any
	const bool         take_prefixes; // Whether temporary prefixes should be removed
};

struct hook_expiry_req
{
	union {
		struct mychan * mc;
		struct myuser * mu;
		struct mynick * mn;
	}                       data;

	int                     do_expire;  // Write zero here to disallow expiry
};

struct hook_host_request
{
	const char *        host;
	struct sourceinfo * si;
	int                 approved;
	const char *        target;
};

struct hook_info_noexist_req
{
	struct sourceinfo * si;
	const char *        nick;
};

struct hook_metadata_change
{
	struct myuser * target;
	const char *    name;
	char *          value;
};

struct hook_module_load
{
	const char *    name;       // The module name we're searching for
	const char *    path;       // The likely path name, which may be ignored
	struct module * module;     // If a module is loaded, set this to point to it
	int             handled;
};

struct hook_myentity_req
{
	struct myentity *   entity;
	const char *        name;
	bool                approval;
};

struct hook_nick_enforce
{
	struct user *   u;
	struct mynick * mn;
};

struct hook_sasl_may_impersonate
{
	struct myuser * source_mu;
	struct myuser * target_mu;
	bool            allowed;
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

struct hook_user_login_check
{
	struct sourceinfo * si;
	struct myuser *     mu;
	bool                allowed;
};

struct hook_user_logout_check
{
	struct sourceinfo * si;
	struct user *       u;
	const bool          relogin;
	bool                allowed;
};

struct hook_user_needforce
{
	struct sourceinfo * si;
	struct myuser *     mu;
	int                 allowed;
};

struct hook_user_nick
{
	struct user *    u;             // User in question. Write NULL here if you delete the user
	const char *     oldnick;       // Previous nick for nick changes. u->nick is the new nick
};

struct hook_user_register_check
{
	struct sourceinfo * si;
	const char *        account;    // or nick
	const char *        email;
	const char *        password;
	int                 approved;   // Write non-zero here to disallow the registration
};

struct hook_user_rename
{
	struct myuser * mu;
	const char *    oldname;
};

struct hook_user_rename_check
{
	struct sourceinfo * si;
	struct myuser *     mu;
	struct mynick *     mn;
	bool                allowed;
};

struct hook_user_req
{
	struct sourceinfo * si;
	struct myuser *     mu;
	struct mynick *     mn;
};

void hook_del_hook(const char *, hook_fn);
void hook_add_hook(const char *, hook_fn);
void hook_add_hook_first(const char *, hook_fn);
void hook_call_event(const char *, void *);

void hook_stop(void);
void hook_continue(void *newptr);

#endif /* !ATHEME_INC_HOOK_H */
