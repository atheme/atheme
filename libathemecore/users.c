/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 * Copyright (C) 2015-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * users.c: User management.
 */

#include <atheme.h>
#include "internal.h"

static mowgli_heap_t *user_heap = NULL;

mowgli_patricia_t *userlist;
mowgli_patricia_t *uidlist;

char hostmaskbuf[NICKLEN + USERLEN + HOSTLEN + 2];

static void
user_delete_cb(void *const restrict user)
{
	(void) user_delete(user, NULL);
}

/*
 * init_users()
 *
 * Initializes the users heap and DTree.
 *
 * Inputs:
 *     - none
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - the users heap and DTree are initialized.
 */
void
init_users(void)
{
	user_heap = sharedheap_get(sizeof(struct user));

	if (user_heap == NULL)
	{
		slog(LG_DEBUG, "init_users(): block allocator failure.");
		exit(EXIT_FAILURE);
	}

	userlist = mowgli_patricia_create(irccasecanon);
	uidlist = mowgli_patricia_create(noopcanon);
}

/*
 * user_add(const char *nick, const char *user, const char *host, const char *vhost, const char *ip,
 *          const char *uid, const char *gecos, struct server *server, time_t ts);
 *
 * User object factory.
 *
 * Inputs:
 *     - nickname of new user
 *     - username of new user
 *     - hostname of new user
 *     - virtual hostname of new user if applicable otherwise NULL
 *     - ip of user if applicable otherwise NULL
 *     - unique identifier (UID) of user if appliable otherwise NULL
 *     - gecos of new user
 *     - pointer to server new user is on
 *     - user's timestamp
 *
 * Outputs:
 *     - on success, a new user
 *     - on failure, NULL
 *
 * Side Effects:
 *     - if successful, a user is created and added to the users DTree.
 *     - if unsuccessful, a kill has been sent if necessary
 */
struct user *
user_add(const char *nick, const char *user, const char *host,
	const char *vhost, const char *ip, const char *uid, const char *gecos,
	struct server *server, time_t ts)
{
	struct user *u, *u2;
	struct hook_user_nick hdata;

	slog(LG_DEBUG, "user_add(): %s (%s@%s) -> %s", nick, user, host, server->name);

	u2 = user_find_named(nick);
	if (u2 != NULL)
	{
		if (server == me.me)
		{
			/* caller should not let this happen */
			slog(LG_ERROR, "user_add(): tried to add local nick %s which already exists", nick);
			return NULL;
		}
		slog(LG_INFO, "user_add(): nick collision on %s", nick);
		if (u2->server == me.me)
		{
			if (uid != NULL)
			{
				/* If the new client has a UID, our
				 * client will have a UID too and the
				 * remote server will send us a kill
				 * if it kills our client.  So just kill
				 * their client and continue.
				 */
				kill_id_sts(NULL, uid, "Nick collision with services (new)");
				return NULL;
			}
			if (ts == u2->ts || ((ts < u2->ts) ^ (!irccasecmp(user, u2->user) && !irccasecmp(host, u2->host))))
			{
				/* If the TSes are equal, or if their TS
				 * is less than our TS and the u@h differs,
				 * or if our TS is less than their TS and
				 * the u@h is equal, our client will be
				 * killed.
				 *
				 * Hope that a kill has arrived just before
				 * for our client; we will have reintroduced
				 * it.
				 */
				return NULL;
			}
			else /* Our client will not be killed. */
				return NULL;
		}
		else
		{
			wallops("Server %s is introducing nick %s which already exists on %s",
					server->name, nick, u2->server->name);
			if (uid != NULL && u2->uid != NULL)
			{
				kill_id_sts(NULL, uid, "Ghost detected via nick collision (new)");
				kill_id_sts(NULL, u2->uid, "Ghost detected via nick collision (old)");
				user_delete(u2, "Ghost detected via nick collision (old)");
			}
			else
			{
				/* There is no way we can do this properly. */
				kill_id_sts(NULL, nick, "Ghost detected via nick collision");
				user_delete(u2, "Ghost detected via nick collision");
			}
			return NULL;
		}
	}

	u = mowgli_heap_alloc(user_heap);
	atheme_object_init(atheme_object(u), nick, &user_delete_cb);

	if (uid != NULL)
	{
		u->uid = strshare_get(uid);
		mowgli_patricia_add(uidlist, u->uid, u);
	}

	u->nick = strshare_get(nick);
	u->user = strshare_get(user);
	u->host = strshare_get(host);
	u->gecos = strshare_get(gecos);
	u->chost = strshare_get(vhost ? vhost : host);
	u->vhost = strshare_get(vhost ? vhost : host);

	if (ip && strcmp(ip, "0") && strcmp(ip, "0.0.0.0") && strcmp(ip, "255.255.255.255"))
		u->ip = strshare_get(ip);

	u->server = server;
	u->server->users++;
	mowgli_node_add(u, &u->snode, &u->server->userlist);

	u->ts = ts ? ts : CURRTIME;

	mowgli_patricia_add(userlist, u->nick, u);

	cnt.user++;

	hdata.u = u;
	hdata.oldnick = NULL;
	hook_call_user_add(&hdata);

	return hdata.u;
}

/*
 * user_delete(struct user *u, const char *comment)
 *
 * Destroys a user object and deletes the object from the users DTree.
 *
 * Inputs:
 *     - user object to delete
 *     - quit comment
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - on success, a user is deleted from the users DTree.
 */
void
user_delete(struct user *u, const char *comment)
{
	mowgli_node_t *n, *tn;
	struct chanuser *cu;
	struct mynick *mn;
	char oldnick[NICKLEN + 1];
	bool doenforcer = false;

	return_if_fail(u != NULL);

	if (u->flags & UF_DOENFORCE)
	{
		doenforcer = true;
		mowgli_strlcpy(oldnick, u->nick, sizeof oldnick);
		u->flags &= ~UF_DOENFORCE;
	}

	if (!comment)
		comment = "";

	slog(LG_DEBUG, "user_delete(): removing user: %s -> %s (%s)", u->nick, u->server->name, comment);

	hook_call_user_delete_info((&(struct hook_user_delete_info){.u = u, .comment = comment}));
	hook_call_user_delete(u);

	u->server->users--;
	if (is_ircop(u))
		u->server->opers--;
	if (u->flags & UF_INVIS)
		u->server->invis--;

	sfree(u->certfp);

	/* remove the user from each channel */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, u->channels.head)
	{
		cu = (struct chanuser *)n->data;

		chanuser_delete(cu->chan, u);
	}

	mowgli_patricia_delete(userlist, u->nick);

	if (u->uid != NULL)
		mowgli_patricia_delete(uidlist, u->uid);

	mowgli_node_delete(&u->snode, &u->server->userlist);

	if (u->myuser)
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, u->myuser->logins.head)
		{
			if (n->data == u)
			{
				mowgli_node_delete(n, &u->myuser->logins);
				mowgli_node_free(n);
				break;
			}
		}
		u->myuser->lastlogin = CURRTIME;
		if ((mn = mynick_find(u->nick)) != NULL &&
				mn->owner == u->myuser)
			mn->lastseen = CURRTIME;
		u->myuser = NULL;
	}

	strshare_unref(u->uid);
	strshare_unref(u->nick);
	strshare_unref(u->user);
	strshare_unref(u->host);
	strshare_unref(u->gecos);
	strshare_unref(u->vhost);
	strshare_unref(u->chost);
	strshare_unref(u->ip);

	mowgli_heap_free(user_heap, u);

	cnt.user--;

	if (doenforcer)
		introduce_enforcer(oldnick);
}

/*
 * user_find(const char *nick)
 *
 * Finds a user by UID or nickname.
 *
 * Inputs:
 *     - nickname or UID to look up
 *
 * Outputs:
 *     - on success, the user object that was requested
 *     - on failure, NULL
 *
 * Side Effects:
 *     - none
 */
struct user *
user_find(const char *nick)
{
	struct user *u;

	return_val_if_fail(nick != NULL, NULL);

	if (ircd->uses_uid)
	{
		u = mowgli_patricia_retrieve(uidlist, nick);

		if (u != NULL)
			return u;
	}

	u = mowgli_patricia_retrieve(userlist, nick);

	if (u != NULL)
	{
		if (ircd->uses_p10)
			wallops("user_find(): found user %s by nick!", nick);
		return u;
	}

	return NULL;
}

/*
 * user_find_named(const char *nick)
 *
 * Finds a user by nickname. Prevents chasing users by their UID.
 *
 * Inputs:
 *     - nickname to look up
 *
 * Outputs:
 *     - on success, the user object that was requested
 *     - on failure, NULL
 *
 * Side Effects:
 *     - none
 */
struct user *
user_find_named(const char *nick)
{
	return mowgli_patricia_retrieve(userlist, nick);
}

/*
 * user_changeuid(struct user *u, const char *uid)
 *
 * Changes a user object's UID.
 *
 * Inputs:
 *     - user object to change UID
 *     - new UID
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - a user object's UID is changed.
 */
void
user_changeuid(struct user *u, const char *uid)
{
	return_if_fail(u != NULL);

	if (u->uid != NULL)
		mowgli_patricia_delete(uidlist, u->uid);

	strshare_unref(u->uid);
	u->uid = strshare_get(uid);

	if (u->uid != NULL)
		mowgli_patricia_add(uidlist, u->uid, u);
}

/*
 * user_changenick(struct user *u, const char *uid)
 *
 * Changes a user object's nick and TS.
 *
 * Inputs:
 *     - user object to change
 *     - new nick
 *     - new TS
 *
 * Outputs:
 *     - whether the user was killed as result of the nick change
 *
 * Side Effects:
 *     - a user object's nick and TS is changed.
 *     - in event of a collision or a decision by a hook, the user may be killed
 */
bool
user_changenick(struct user *u, const char *nick, time_t ts)
{
	struct mynick *mn;
	struct user *u2;
	char oldnick[NICKLEN + 1];
	bool doenforcer = false;
	struct hook_user_nick hdata;

	return_val_if_fail(u != NULL, false);
	return_val_if_fail(nick != NULL, false);

	mowgli_strlcpy(oldnick, u->nick, sizeof oldnick);
	u2 = user_find_named(nick);
	if (u->flags & UF_DOENFORCE && u2 != u)
	{
		doenforcer = true;
		u->flags &= ~UF_DOENFORCE;
	}
	if (u2 != NULL && u2 != u)
	{
		if (u->server == me.me)
		{
			/* caller should not let this happen */
			slog(LG_ERROR, "user_changenick(): tried to change local nick %s to %s which already exists", u->nick, nick);
			return false;
		}
		slog(LG_INFO, "user_changenick(): nick collision on %s", nick);
		if (u2->server == me.me)
		{
			if (u->uid != NULL)
			{
				/* If the changing client has a UID, our
				 * client will have a UID too and the
				 * remote server will send us a kill
				 * if it kills our client.  So just kill
				 * their client and continue.
				 */
				kill_id_sts(NULL, u->uid, "Nick change collision with services");
				user_delete(u, "Nick change collision with services");
				return true;
			}
			if (ts == u2->ts || ((ts < u2->ts) ^ (!irccasecmp(u->user, u2->user) && !irccasecmp(u->host, u2->host))))
			{
				/* If the TSes are equal, or if their TS
				 * is less than our TS and the u@h differs,
				 * or if our TS is less than their TS and
				 * the u@h is equal, our client will be
				 * killed.
				 *
				 * Hope that a kill has arrived just before
				 * for our client; we will have reintroduced
				 * it.
				 * But kill the changing client using its
				 * old nick.
				 */
				kill_id_sts(NULL, u->nick, "Nick change collision with services");
				user_delete(u, "Nick change collision with services");
				return true;
			}
			else
			{
				/* Our client will not be killed.
				 * But kill the changing client using its
				 * old nick.
				 */
				kill_id_sts(NULL, u->nick, "Nick change collision with services");
				user_delete(u, "Nick change collision with services");
				return true;
			}
		}
		else
		{
			wallops("Server %s is sending nick change from %s to %s which already exists on %s",
					u->server->name, u->nick, nick,
					u2->server->name);
			if (u->uid != NULL && u2->uid != NULL)
			{
				kill_id_sts(NULL, u->uid, "Ghost detected via nick change collision (new)");
				kill_id_sts(NULL, u2->uid, "Ghost detected via nick change collision (old)");
				user_delete(u, "Ghost detected via nick change collision (new)");
				user_delete(u2, "Ghost detected via nick change collision (old)");
			}
			else
			{
				/* There is no way we can do this properly. */
				kill_id_sts(NULL, u->nick, "Ghost detected via nick change collision");
				kill_id_sts(NULL, nick, "Ghost detected via nick change collision");
				user_delete(u, "Ghost detected via nick change collision");
				user_delete(u2, "Ghost detected via nick change collision");
			}
			return true;
		}
	}
	if (u->myuser != NULL && (mn = mynick_find(u->nick)) != NULL &&
			mn->owner == u->myuser)
		mn->lastseen = CURRTIME;
	mowgli_patricia_delete(userlist, u->nick);

	strshare_unref(u->nick);
	u->nick = strshare_get(nick);

	u->ts = ts;

	mowgli_patricia_add(userlist, u->nick, u);

	if (doenforcer)
		introduce_enforcer(oldnick);

	hdata.u = u;
	hdata.oldnick = oldnick;
	hook_call_user_nickchange(&hdata);

	return hdata.u == NULL;
}

/*
 * user_mode(struct user *user, const char *modes)
 *
 * Changes a user object's modes.
 *
 * Inputs:
 *     - user object to change modes on
 *     - modestring describing the usermode change
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - on success, a user's modes are adjusted.
 *
 * Bugs:
 *     - this routine only tracks +i and +o usermode changes.
 */
void
user_mode(struct user *user, const char *modes)
{
	int dir = MTYPE_ADD;
	bool was_ircop, was_invis;
	int iter;

	return_if_fail(user != NULL);
	return_if_fail(modes != NULL);

	was_ircop = is_ircop(user);
	was_invis = (user->flags & UF_INVIS);

	while (*modes != '\0')
	{
		switch (*modes)
		{
		  case '+':
			  dir = MTYPE_ADD;
			  break;
		  case '-':
			  dir = MTYPE_DEL;
			  break;
		  default:
			  for (iter = 0; user_mode_list[iter].mode != '\0'; iter++)
			  {
				  if (*modes == user_mode_list[iter].mode)
				  {
					  if (dir == MTYPE_ADD)
						  user->flags |= user_mode_list[iter].value;
					  else
						  user->flags &= ~user_mode_list[iter].value;
				  }
			  }
			  break;
		}
		modes++;
	}

	/* update stats and do appropriate hooks... */
	if (!was_invis && (user->flags & UF_INVIS))
		user->server->invis++;
	else if (was_invis && !(user->flags & UF_INVIS))
		user->server->invis--;

	if (!was_ircop && is_ircop(user))
	{
		slog(LG_DEBUG, "user_mode(): %s is now an IRCop", user->nick);
		slog(LG_INFO, "OPER: \2%s\2 (\2%s\2)", user->nick, user->server->name);
		user->server->opers++;
		hook_call_user_oper(user);
	}
	else if (was_ircop && !is_ircop(user))
	{
		slog(LG_DEBUG, "user_mode(): %s is no longer an IRCop", user->nick);
		slog(LG_INFO, "DEOPER: \2%s\2 (\2%s\2)", user->nick, user->server->name);
		user->server->opers--;
		hook_call_user_deoper(user);
	}
}

/*
 * user_sethost(struct user *source, struct user *target, const char *host)
 *
 * Sets a new virtual host on a user.
 *
 * Inputs:
 *     - source user (e.g. what bot is setting the host)
 *     - target user
 *     - new virtual host
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - virtual host is set
 */
void
user_sethost(struct user *source, struct user *target, stringref host)
{
	return_if_fail(source != NULL);
	return_if_fail(target != NULL);
	return_if_fail(host != NULL);

	if (*host == '\0')
	{
		slog(LG_INFO, "user_sethost(): eh?  trying to set a blank vhost on %s", target->nick);
		return;
	}

	strshare_unref(target->vhost);
	target->vhost = strshare_get(host);

	sethost_sts(source, target, target->vhost);
	hook_call_user_sethost(target);
}

const char *
user_get_umodestr(struct user *u)
{
	static char result[34];
	int iter;
	int i;

	return_val_if_fail(u != NULL, NULL);

	i = 0;
	result[i++] = '+';
	for (iter = 0; user_mode_list[iter].mode != '\0'; iter++)
		if (u->flags & user_mode_list[iter].value)
			result[i++] = user_mode_list[iter].mode;
	result[i] = '\0';
	return result;
}

struct chanuser *
find_user_banned_channel(struct user *const restrict u, const char ban_type)
{
	mowgli_node_t *n;

	return_val_if_fail(u != NULL, false);
	return_val_if_fail(ban_type != 0, false);

	MOWGLI_ITER_FOREACH(n, u->channels.head)
	{
		struct chanuser *cu = n->data;

		/* Assume that any prefix modes allow changing nicks even while banned */
		if (cu->modes != 0)
			continue;

		if (next_matching_ban(cu->chan, u, ban_type, cu->chan->bans.head) != NULL)
		{
			if (ircd->except_mchar == '\0' || next_matching_ban(cu->chan, u, ircd->except_mchar, cu->chan->bans.head) == NULL)
				return cu;
			else
				continue;
		}
	}

	return NULL;
}

const char *
format_hostmask(const struct user *user, const enum host_type type)
{
	sprintf(hostmaskbuf, "%s!%s!%s", user->nick, user->user, user->vhost);
	return hostmaskbuf;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
