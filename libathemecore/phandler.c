/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2015 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * phandler.c: Generic protocol handling routines.
 */

#include <atheme.h>
#include "internal.h"

unsigned int(*server_login) (void) = generic_server_login;
void (*introduce_nick) (struct user *u) = generic_introduce_nick;
void (*wallops_sts) (const char *text) = generic_wallops_sts;
void (*join_sts) (struct channel *c, struct user *u, bool isnew, char *modes) = generic_join_sts;
void (*chan_lowerts) (struct channel *c, struct user *u) = generic_chan_lowerts;
void (*kick) (struct user *source, struct channel *c, struct user *u, const char *reason) = generic_kick;
void (*msg) (const char *from, const char *target, const char *fmt, ...) = generic_msg;
void (*msg_global_sts) (struct user *from, const char *mask, const char *text) = generic_msg_global_sts;
void (*notice_user_sts) (struct user *from, struct user *target, const char *text) = generic_notice_user_sts;
void (*notice_global_sts) (struct user *from, const char *mask, const char *text) = generic_notice_global_sts;
void (*notice_channel_sts) (struct user *from, struct channel *target, const char *text) = generic_notice_channel_sts;
void (*wallchops) (struct user *source, struct channel *target, const char *message) = generic_wallchops;
void (*numeric_sts) (struct server *from, int numeric, struct user *target, const char *fmt, ...) = generic_numeric_sts;
void (*kill_id_sts) (struct user *killer, const char *id, const char *reason) = generic_kill_id_sts;
void (*part_sts) (struct channel *c, struct user *u) = generic_part_sts;
void (*kline_sts) (const char *server, const char *user, const char *host, long duration, const char *reason) = generic_kline_sts;
void (*unkline_sts) (const char *server, const char *user, const char *host) = generic_unkline_sts;
void (*xline_sts) (const char *server, const char *realname, long duration, const char *reason) = generic_xline_sts;
void (*unxline_sts) (const char *server, const char *realname) = generic_unxline_sts;
void (*qline_sts) (const char *server, const char *mask, long duration, const char *reason) = generic_qline_sts;
void (*unqline_sts) (const char *server, const char *mask) = generic_unqline_sts;
void (*topic_sts) (struct channel *c, struct user *source, const char *setter, time_t ts, time_t prevts, const char *topic) = generic_topic_sts;
void (*mode_sts) (char *sender, struct channel *target, char *modes) = generic_mode_sts;
void (*ping_sts) (void) = generic_ping_sts;
void (*quit_sts) (struct user *u, const char *reason) = generic_quit_sts;
void (*ircd_on_login) (struct user *u, struct myuser *account, const char *wantedhost) = generic_on_login;
bool (*ircd_on_logout) (struct user *u, const char *account) = generic_on_logout;
void (*jupe) (const char *server, const char *reason) = generic_jupe;
void (*sethost_sts) (struct user *source, struct user *target, const char *host) = generic_sethost_sts;
void (*fnc_sts) (struct user *source, struct user *u, const char *newnick, int type) = generic_fnc_sts;
void (*holdnick_sts)(struct user *source, int duration, const char *nick, struct myuser *account) = generic_holdnick_sts;
void (*invite_sts) (struct user *source, struct user *target, struct channel *channel) = generic_invite_sts;
void (*svslogin_sts) (const char *target, const char *nick, const char *user, const char *host, struct myuser *account) = generic_svslogin_sts;
void (*sasl_sts) (const char *target, char mode, const char *data) = generic_sasl_sts;
void (*sasl_mechlist_sts) (const char *mechlist) = generic_sasl_mechlist_sts;
bool (*mask_matches_user)(const char *mask, struct user *u) = generic_mask_matches_user;
mowgli_node_t *(*next_matching_ban)(struct channel *c, struct user *u, int type, mowgli_node_t *first) = generic_next_matching_ban;
mowgli_node_t *(*next_matching_host_chanacs)(struct mychan *mc, struct user *u, mowgli_node_t *first) = generic_next_matching_host_chanacs;
bool (*is_valid_nick)(const char *nick) = generic_is_valid_nick;
bool (*is_valid_host)(const char *host) = generic_is_valid_host;
bool (*is_valid_username)(const char *username) = generic_is_valid_username;
void (*mlock_sts)(struct channel *c) = generic_mlock_sts;
void (*topiclock_sts)(struct channel *c) = generic_topiclock_sts;
void (*quarantine_sts)(struct user *source, struct user *victim, long duration, const char *reason) = generic_quarantine_sts;
bool (*is_extban)(const char *mask) = generic_is_extban;
void (*dline_sts)(const char *server, const char *host, long duration, const char *reason) = generic_dline_sts;
void (*undline_sts)(const char *server, const char *host) = generic_undline_sts;
bool (*is_ircop)(struct user *u) = generic_is_ircop;
bool (*is_admin)(struct user *u) = generic_is_admin;
bool (*is_service)(struct user *u) = generic_is_service;

unsigned int
generic_server_login(void)
{
	/* Nothing to do here. */
	return 0;
}

void
generic_introduce_nick(struct user *u)
{
	/* Nothing to do here. */
}

void
generic_wallops_sts(const char *text)
{
	/* ugly, but some ircds offer no alternative -- jilles */
	struct user *u;
	mowgli_patricia_iteration_state_t state;
	char buf[BUFSIZE];

	snprintf(buf, sizeof buf, "*** Notice -- %s", text);
	MOWGLI_PATRICIA_FOREACH(u, &state, userlist)
	{
		if (!is_internal_client(u) && is_ircop(u))
			notice_user_sts(NULL, u, buf);
	}
}

void
generic_join_sts(struct channel *c, struct user *u, bool isnew, char *modes)
{
	/* We can't do anything here. Bail. */
}

void
generic_chan_lowerts(struct channel *c, struct user *u)
{
	slog(LG_ERROR, "chan_lowerts() called but not supported!");
	join_sts(c, u, true, channel_modes(c, true));
}

void
generic_kick(struct user *source, struct channel *c, struct user *u, const char *reason)
{
	/* We can't do anything here. Bail. */
}

void ATHEME_FATTR_PRINTF(3, 4)
generic_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	if (vsnprintf(buf, sizeof buf, fmt, ap) < 0)
	{
		va_end(ap);
		return;
	}
	va_end(ap);

	slog(LG_INFO, "Cannot send message to %s (%s): don't know how. Load a protocol module perhaps?", target, buf);
}

void
generic_msg_global_sts(struct user *from, const char *mask, const char *text)
{
	slog(LG_INFO, "Cannot send global message to %s (%s): don't know how. Load a protocol module perhaps?", mask, text);
}

void
generic_notice_user_sts(struct user *from, struct user *target, const char *text)
{
	slog(LG_INFO, "Cannot send notice to %s (%s): don't know how. Load a protocol module perhaps?", target->nick, text);
}

void
generic_notice_global_sts(struct user *from, const char *mask, const char *text)
{
	slog(LG_INFO, "Cannot send global notice to %s (%s): don't know how. Load a protocol module perhaps?", mask, text);
}

void
generic_notice_channel_sts(struct user *from, struct channel *target, const char *text)
{
	slog(LG_INFO, "Cannot send notice to %s (%s): don't know how. Load a protocol module perhaps?", target->name, text);
}

void
generic_wallchops(struct user *sender, struct channel *channel, const char *message)
{
	/* ugly, but always works -- jilles */
	mowgli_node_t *n;
	struct chanuser *cu;

	MOWGLI_ITER_FOREACH(n, channel->members.head)
	{
		cu = (struct chanuser *)n->data;
		if (cu->user->server != me.me && cu->modes & CSTATUS_OP)
			notice(sender->nick, cu->user->nick, "[@%s] %s", channel->name, message);
	}
}

void ATHEME_FATTR_PRINTF(4, 5)
generic_numeric_sts(struct server *from, int numeric, struct user *target, const char *fmt, ...)
{
	va_list va;
	char buf[BUFSIZE];

	va_start(va, fmt);
	if (vsnprintf(buf, sizeof buf, fmt, va) < 0)
	{
		va_end(va);
		return;
	}
	va_end(va);

	sts(":%s %d %s %s", SERVER_NAME(from), numeric, CLIENT_NAME(target), buf);
}

void
generic_kill_id_sts(struct user *killer, const char *id, const char *reason)
{
	/* cant do anything here. bail. */
}

void
generic_part_sts(struct channel *c, struct user *u)
{
	/* cant do anything here. bail. */
}

void
generic_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason)
{
	/* cant do anything here. bail. */
}

void
generic_unkline_sts(const char *server, const char *user, const char *host)
{
	/* cant do anything here. bail. */
}

void
generic_xline_sts(const char *server, const char *realname, long duration, const char *reason)
{
	/* cant do anything here. bail. */
}

void
generic_unxline_sts(const char *server, const char *realname)
{
	/* cant do anything here. bail. */
}

void
generic_qline_sts(const char *server, const char *mask, long duration, const char *reason)
{
	/* cant do anything here. bail. */
}

void
generic_unqline_sts(const char *server, const char *mask)
{
	/* cant do anything here. bail. */
}

void
generic_dline_sts(const char *server, const char *host, long duration, const char *reason)
{
	/* can't do anything here. bail. */
}

void
generic_undline_sts(const char *server, const char *host)
{
	/* can't do anything here. bail. */
}

void
generic_topic_sts(struct channel *c, struct user *source, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	/* cant do anything here. bail. */
}

void
generic_mode_sts(char *sender, struct channel *target, char *modes)
{
	/* cant do anything here. bail. */
}

void
generic_ping_sts(void)
{
	/* cant do anything here. bail. */
}

void
generic_quit_sts(struct user *u, const char *reason)
{
	/* cant do anything here. bail. */
}

void
generic_on_login(struct user *u, struct myuser *account, const char *wantedhost)
{
	/* nothing to do here. */
}

bool
generic_on_logout(struct user *u, const char *account)
{
	/* nothing to do here. */
	return false;
}

void
generic_jupe(const char *server, const char *reason)
{
	/* nothing to do here. */
}

void
generic_sethost_sts(struct user *source, struct user *target, const char *host)
{
	/* nothing to do here. */
}

void
generic_fnc_sts(struct user *source, struct user *u, const char *newnick, int type)
{
	if (type == FNC_FORCE) /* XXX this does not work properly */
		kill_id_sts(source, CLIENT_NAME(u), "Nickname enforcement");
}

void
generic_holdnick_sts(struct user *source, int duration, const char *nick, struct myuser *account)
{
	/* nothing to do here. */
}

void
generic_invite_sts(struct user *source, struct user *target, struct channel *channel)
{
	/* nothing to do here. */
}

void
generic_svslogin_sts(const char *target, const char *nick, const char *user, const char *host, struct myuser *account)
{
	/* nothing to do here. */
}

void
generic_sasl_sts(const char *target, char mode, const char *data)
{
	/* nothing to do here. */
}

void
generic_sasl_mechlist_sts(const char *mechlist)
{
	/* nothing to do here. */
}

bool
generic_mask_matches_user(const char *mask, struct user *u)
{
	char hostbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char cloakbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char realbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];
	char ipbuf[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1];

	snprintf(hostbuf, sizeof hostbuf, "%s!%s@%s", u->nick, u->user, u->vhost);
	snprintf(cloakbuf, sizeof cloakbuf, "%s!%s@%s", u->nick, u->user, u->chost);

	bool result = !match(mask, hostbuf) || !match(mask, cloakbuf);

	// return if configured not to check further, or if we already have a match
	if ((!config_options.masks_through_vhost && u->host != u->vhost) || result)
		return result;

	snprintf(realbuf, sizeof realbuf, "%s!%s@%s", u->nick, u->user, u->host);
	/* will be nick!user@ if ip unknown, doesn't matter */
	snprintf(ipbuf, sizeof ipbuf, "%s!%s@%s", u->nick, u->user, u->ip);

	return !match(mask, realbuf) || !match(mask, ipbuf) || (ircd->flags & IRCD_CIDR_BANS && !match_cidr(mask, ipbuf));

}

mowgli_node_t *
generic_next_matching_ban(struct channel *c, struct user *u, int type, mowgli_node_t *first)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, first)
	{
		struct chanban *cb = n->data;

		if (cb->type == type && mask_matches_user(cb->mask, u))
			return n;
	}
	return NULL;
}

mowgli_node_t *
generic_next_matching_host_chanacs(struct mychan *mc, struct user *u, mowgli_node_t *first)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, first)
	{
		struct chanacs *ca = n->data;

		if (ca->entity != NULL)
		       continue;
		if (mask_matches_user(ca->host, u))
			return n;
	}
	return NULL;
}

bool
generic_is_valid_nick(const char *nick)
{
	/* nicknames may not begin with a digit or a hyphen; the former because they are usually
	 * reserved for UIDs and thus may collide, and both because it is specified as such in
	 * <https://datatracker.ietf.org/doc/html/rfc2812#section-2.3.1>
	 */
	if (IsDigit(*nick) || *nick == '-')
		return false;

	// nicknames cannot be longer than we support
	if (strlen(nick) > NICKLEN)
		return false;

	for (const char *iter = nick; *iter != '\0'; iter++)
	{
		if (!IsNickChar(*iter))
			return false;
	}

	return true;
}

bool
generic_is_valid_username(const char *username)
{
	const char *iter = username;

	for (; *iter != '\0'; iter++)
	{
		if (!IsUserChar(*iter))
			return false;
	}

	return true;
}

bool
generic_is_valid_host(const char *host)
{
	/* don't know what to do here */
	return true;
}

void
generic_mlock_sts(struct channel *c)
{
	/* nothing to do here. */
}

void
generic_topiclock_sts(struct channel *c)
{
	/* nothing to do here. */
}

void
generic_quarantine_sts(struct user *source, struct user *victim, long duration, const char *reason)
{
	/* nothing to do here */
}

bool
generic_is_extban(const char *mask)
{
	return false;
}

bool
generic_is_ircop(struct user *u)
{
	if (UF_IRCOP & u->flags)
		return true;

	return false;
}

bool
generic_is_admin(struct user *u)
{
	if (UF_ADMIN & u->flags)
		return true;

	return false;
}

bool
generic_is_service(struct user *u)
{
	if (UF_SERVICE & u->flags)
		return true;

	return false;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
