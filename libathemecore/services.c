/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2015 Atheme Project (http://atheme.org/)
 * Copyright (C) 2016-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * services.c: Routines commonly used by various services.
 */

#include <atheme.h>
#include "internal.h"

static mowgli_heap_t *sourceinfo_heap = NULL;

int authservice_loaded = 0;
int use_myuser_access = 0;
int use_svsignore = 0;
int use_privmsg = 0;
int use_account_private = 1;
int use_channel_private = 1;
int use_limitflags = 0;

#define MAX_BUF 256

/* ban wrapper for cmode, returns number of bans added (0 or 1) */
unsigned int
ban(struct user *sender, struct channel *c, struct user *user)
{
	char mask[MAX_BUF];
	struct chanban *cb;

	if (!c)
		return 0;

	snprintf(mask, MAX_BUF, "*!*@%s", user->vhost);
	mask[MAX_BUF - 1] = '\0';

	cb = chanban_find(c, mask, 'b');

	if (cb != NULL)
		return 0;

	chanban_add(c, mask, 'b');

	modestack_mode_param(sender->nick, c, MTYPE_ADD, 'b', mask);
	modestack_flush_now();

	return 1;
}

/* returns number of modes removed -- jilles */
unsigned int
remove_banlike(struct user *source, struct channel *chan, int type, struct user *target)
{
	unsigned int count = 0;
	mowgli_node_t *n, *tn;
	struct chanban *cb;

	if (type == 0)
		return 0;
	if (source == NULL || chan == NULL || target == NULL)
		return 0;

	for (n = next_matching_ban(chan, target, type, chan->bans.head); n != NULL; n = next_matching_ban(chan, target, type, tn))
	{
		tn = n->next;
		cb = n->data;

		modestack_mode_param(source->nick, chan, MTYPE_DEL, cb->type, cb->mask);
		chanban_delete(cb);
		count++;
	}

	modestack_flush_now();

	return count;
}

/* returns number of exceptions removed -- jilles */
unsigned int
remove_ban_exceptions(struct user *source, struct channel *chan, struct user *target)
{
	return remove_banlike(source, chan, ircd->except_mchar, target);
}

// returns true if user was actually kicked, false otherwise (or on assertion failure)
// If the user was kicked, their chanuser is deleted; if the user was *not* kicked,
// their chanuser remains. This currently only happens due to kick immunity.
bool
try_kick_real(struct user *source, struct channel *chan, struct user *target, const char *reason)
{
	return_val_if_fail(source != NULL, false);
	return_val_if_fail(chan != NULL, false);
	return_val_if_fail(target != NULL, false);
	return_val_if_fail(reason != NULL, false);

	struct chanuser *cu = chanuser_find(chan, target);

	return_val_if_fail(cu != NULL, false);

	if ((chan->modes & ircd->oimmune_mode || cu->modes & CSTATUS_IMMUNE) && is_ircop(target))
	{
		wallops("Not kicking oper %s!%s@%s from protected %s (%s: %s)",
				target->nick, target->user, target->vhost,
				chan->name, source->nick, reason);
		notice(source->nick, chan->name,
				"Not kicking oper %s (%s)",
				target->nick, reason);
		return false;
	}
	if (target->flags & config_options.immune_level)
	{
		wallops("Not kicking immune user %s!%s@%s from %s (%s: %s)",
				target->nick, target->user, target->vhost,
				chan->name, source->nick, reason);
		notice(source->nick, chan->name,
				"Not kicking immune user %s (%s)",
				target->nick, reason);
		return false;
	}
	kick(source, chan, target, reason);
	return true;
}

bool (*try_kick)(struct user *source, struct channel *chan, struct user *target, const char *reason) = try_kick_real;

/* sends a KILL message for a user and removes the user from the userlist
 * source should be a service user or NULL for a server kill
 */
void ATHEME_FATTR_PRINTF(3, 4)
kill_user(struct user *source, struct user *victim, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	char qreason[512];

	return_if_fail(victim != NULL);

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	if (victim->flags & UF_ENFORCER)
		quit_sts(victim, buf);
	else if (victim->server == me.me)
	{
		slog(LG_INFO, "kill_user(): not killing service %s (source %s, comment %s)",
				victim->nick,
				source != NULL ? source->nick : me.name,
				buf);
		return;
	}
	else
		kill_id_sts(source, CLIENT_NAME(victim), buf);
	snprintf(qreason, sizeof qreason, "Killed (%s (%s))",
			source != NULL ? source->nick : me.name, buf);
	user_delete(victim, qreason);
}

void
introduce_enforcer(const char *nick)
{
	struct user *u;

	/* TS 1 to win nick collisions */
	u = user_add(nick, "enforcer", me.name, NULL, NULL,
			ircd->uses_uid ? uid_get() : NULL,
			"Held for nickname owner", me.me, 1);
	return_if_fail(u != NULL);
	u->flags |= UF_INVIS | UF_ENFORCER;
	introduce_nick(u);
}

/* join a channel, creating it if necessary */
void
join(const char *chan, const char *nick)
{
	struct channel *c;
	struct user *u;
	struct chanuser *cu;
	bool isnew = false;
	struct mychan *mc;
	struct metadata *md;
	time_t ts;

	u = user_find_named(nick);
	if (!u)
		return;
	c = channel_find(chan);
	if (c == NULL)
	{
		mc = mychan_find(chan);
		if (chansvs.changets && mc != NULL)
		{
			/* Use the previous TS if known, registration
			 * time otherwise, but never ever create a channel
			 * with TS 0 -- jilles */
			ts = mc->registered;
			md = metadata_find(mc, "private:channelts");
			if (md != NULL)
				ts = atol(md->value);
			if (ts == 0)
				ts = CURRTIME;
		}
		else
			ts = CURRTIME;
		c = channel_add(chan, ts, me.me);
		c->modes |= CMODE_NOEXT | CMODE_TOPIC;
		if (mc != NULL)
			check_modes(mc, false);
		isnew = true;
	}
	else if (chanuser_find(c, u))
	{
		slog(LG_DEBUG, "join(): i'm already in `%s'", c->name);
		return;
	}
	join_sts(c, u, isnew, channel_modes(c, true));
	cu = chanuser_add(c, CLIENT_NAME(u));
	cu->modes |= CSTATUS_OP;
	if (isnew)
	{
		hook_call_channel_add(c);
	}
}

/* part a channel */
void
part(const char *chan, const char *nick)
{
	struct channel *c = channel_find(chan);
	struct user *u = user_find_named(nick);

	if (!u || !c)
		return;
	if (!chanuser_find(c, u))
		return;
	if (me.connected)
		part_sts(c, u);
	chanuser_delete(c, u);
}

void
services_init(void)
{
	struct service *svs;
	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(svs, &state, services_name)
	{
		if (ircd->uses_uid && svs->me->uid == NULL)
			user_changeuid(svs->me, uid_get());
		else if (!ircd->uses_uid && svs->me->uid != NULL)
			user_changeuid(svs->me, NULL);
		if (!ircd->uses_uid)
			kill_id_sts(NULL, svs->nick, "Attempt to use service nick");
		introduce_nick(svs->me);
	}
}

void
joinall(const char *name)
{
	struct service *svs;
	mowgli_patricia_iteration_state_t state;

	if (name == NULL)
		return;

	MOWGLI_PATRICIA_FOREACH(svs, &state, services_name)
	{
		/* service must be online and not a botserv bot */
		if (svs->me == NULL || svs->botonly)
			continue;
		join(name, svs->me->nick);
	}
}

void
partall(const char *name)
{
	mowgli_patricia_iteration_state_t state;
	struct service *svs;
	struct mychan *mc;

	if (name == NULL)
		return;
	mc = mychan_find(name);
	MOWGLI_PATRICIA_FOREACH(svs, &state, services_name)
	{
		if (svs == chansvs.me && mc != NULL && mc->flags & MC_GUARD)
			continue;
		if (svs->me == NULL)
			continue;
		/* Do not cache this channel_find(), the
		 * channel may disappear under our feet
		 * -- jilles */
		if (chanuser_find(channel_find(name), svs->me))
			part(name, svs->me->nick);
	}
}

/* reintroduce a service e.g. after it's been killed -- jilles */
void
reintroduce_user(struct user *u)
{
	mowgli_node_t *n;
	struct channel *c;
	struct service *svs;

	svs = service_find_nick(u->nick);
	if (svs == NULL)
	{
		slog(LG_DEBUG, "tried to reintroduce_user non-service %s", u->nick);
		return;
	}
	/* Reintroduce with a new UID.  This avoids problems distinguishing
	 * commands targeted at the old and new user.
	 */
	if (u->uid != NULL)
	{
		user_changeuid(u, uid_get());
	}
	else
	{
		/* Ensure it is really gone before introducing the new one.
		 * This also helps with nick collisions.
		 * With UID this is not necessary as we will
		 * reintroduce with a new UID, and nick collisions
		 * are unambiguous.
		 */
		if (!ircd->uses_uid)
			kill_id_sts(NULL, u->nick, "Service nick");
	}
	introduce_nick(u);
	MOWGLI_ITER_FOREACH(n, u->channels.head)
	{
		c = ((struct chanuser *)n->data)->chan;
		if (MOWGLI_LIST_LENGTH(&c->members) > 1 || c->modes & ircd->perm_mode)
			join_sts(c, u, 0, channel_modes(c, true));
		else
		{
			/* channel will have been destroyed... */
			/* XXX resend the bans instead of destroying them? */
			chanban_clear(c);
			join_sts(c, u, 1, channel_modes(c, true));
			if (c->topic != NULL)
				topic_sts(c, chansvs.me->me, c->topic_setter, c->topicts, 0, c->topic);
		}
	}
}

void ATHEME_FATTR_PRINTF(2, 3)
verbose(const struct mychan *mychan, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	if (mychan->chan == NULL)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	if ((MC_VERBOSE | MC_FORCEVERBOSE) & mychan->flags)
		notice(chansvs.nick, mychan->name, "%s", buf);
	else if (MC_VERBOSE_OPS & mychan->flags)
		wallchops(chansvs.me->me, mychan->chan, buf);
}

/* protocol wrapper for nickchange/nick burst */
void
handle_nickchange(struct user *const restrict u)
{
	return_if_fail(u != NULL);
	return_if_fail(!is_internal_client(u));

	const struct service *const svs = service_find("global");
	const char *const source = (svs != NULL) ? svs->me->nick : me.name;

	if (log_debug_enabled())
	{
		const char *const logtarget = (runflags & RF_LIVE) ? "console" : "file or channel";

		(void) notice(source, u->nick, "Services are presently running in debug mode, logging network "
		                               "traffic to a %s. You should take extra caution when utilizing "
		                               "your services passwords.", logtarget);
	}

	if (readonly)
		(void) notice(source, u->nick, "Services are presently running in read-only mode. Any changes "
		                               "you make will not be saved.");

	(void) hook_call_nick_check(u);
}

bool ircd_logout_or_kill(struct user *u, const char *login)
{
	struct hook_user_logout_check req = {
		.si      = NULL,
		.u       = u,
		.allowed = true,
		.relogin = false,
	};

	hook_call_user_can_logout(&req);

	if (req.allowed)
		return ircd_on_logout(u, login);

	kill_user(nicksvs.me ? nicksvs.me->me : NULL, u, "Forcing logout %s -> %s", u->nick, login);
	return true;
}

/* User u is bursted as being logged in to login (if not NULL) or as
 * being identified to their current nick (if login is NULL)
 * The timestamp is the time of registration of the account, if the ircd
 * stores this, or 0 if not known.
 * Update the administration or log them out on ircd
 * How to use this in protocol modules:
 * 1. if login info is bursted in a command that always occurs, call
 *    this if the user is logged in, before handle_nickchange()
 * 2. if it is bursted in a command that doesn't always occur, use
 *    netwide EOB as in the ratbox module; call this if the user is logged
 *    in; for all users, postpone handle_nickchange() until the user's
 *    server confirms EOB
 * -- jilles
 */
void
handle_burstlogin(struct user *u, const char *login, time_t ts)
{
	struct mynick *mn;
	struct myuser *mu;
	mowgli_node_t *n;

	if (login != NULL)
		/* don't allow alias nicks here -- jilles */
		mu = myuser_find(login);
	else
	{
		mn = mynick_find(u->nick);
		mu = mn != NULL ? mn->owner : NULL;
		login = mu != NULL ? entity(mu)->name : u->nick;
	}
	if (mu == NULL)
	{
		/* account dropped during split...
		 * if we have an authentication service, log them out */
		if (authservice_loaded || backend_loaded)
		{
			slog(LG_DEBUG, "handle_burstlogin(): got nonexistent login %s for user %s", login, u->nick);
			if (authservice_loaded)
			{
				notice(nicksvs.nick ? nicksvs.nick : me.name, u->nick, "Account %s dropped, forcing logout", login);
				ircd_logout_or_kill(u, login);
			}
			return;
		}
		/* we're running without a persistent db, create it */
		mu = myuser_add(login, "*", "noemail", MU_CRYPTPASS);
		if (ts != 0)
			mu->registered = ts;
		metadata_add(mu, "fake", "1");
	}
	if (u->myuser != NULL)	/* already logged in, hmm */
		return;
	if (ts != 0 && ts != mu->registered)
	{
		/* wrong account creation time
		 * if we have an authentication service, log them out */
		slog(LG_INFO, "handle_burstlogin(): got stale login %s for user %s", login, u->nick);
		if (authservice_loaded)
		{
			notice(nicksvs.nick ? nicksvs.nick : me.name, u->nick, "Login to account %s is stale, forcing logout", login);
			ircd_logout_or_kill(u, login);
		}
		return;
	}
	if (mu->flags & MU_NOBURSTLOGIN && authservice_loaded)
	{
		/* no splits for this account, this bursted login cannot
		 * be legit...
		 * if we have an authentication service, log them out */
		slog(LG_INFO, "handle_burstlogin(): got illegit login %s for user %s", login, u->nick);
		notice(nicksvs.nick ? nicksvs.nick : me.name, u->nick, "Login to account %s seems invalid, forcing logout", login);
		ircd_logout_or_kill(u, login);
		return;
	}
	u->myuser = mu;
	u->flags &= ~UF_SOPER_PASS;
	n = mowgli_node_create();
	mowgli_node_add(u, n, &mu->logins);
	slog(LG_DEBUG, "handle_burstlogin(): automatically identified %s as %s", u->nick, login);

	/* XXX: ugh, this is a lame hack but I can't think of anything better... --nenolod */
	if (mu->flags & MU_PENDINGLOGIN && authservice_loaded)
	{
		slog(LG_DEBUG, "handle_burstlogin(): handling pending login hooks for %s", u->nick);
		mu->flags &= ~MU_PENDINGLOGIN;
		hook_call_user_identify(u);
	}
}

void
handle_setlogin(struct sourceinfo *si, struct user *u, const char *login, time_t ts)
{
	struct mynick *mn;
	struct myuser *mu;
	mowgli_node_t *n;

	if (login != NULL)
		/* don't allow alias nicks here -- jilles */
		mu = myuser_find(login);
	else
	{
		mn = mynick_find(u->nick);
		mu = mn != NULL ? mn->owner : NULL;
		login = mu != NULL ? entity(mu)->name : u->nick;
	}

	if (authservice_loaded)
		// Non-SASL logins will be ignored; SASL reauth was handled by modules/saslserv/main already
		return;

	if (u->myuser != NULL)
	{
		n = mowgli_node_find(u, &u->myuser->logins);
		if (n != NULL)
		{
			mowgli_node_delete(n, &u->myuser->logins);
			mowgli_node_free(n);
		}
		u->myuser = NULL;
	}
	if (mu == NULL)
	{
		if (backend_loaded)
		{
			slog(LG_DEBUG, "handle_setlogin(): got nonexistent login %s for user %s", login, u->nick);
			return;
		}
		/* we're running without a persistent db, create it */
		mu = myuser_add(login, "*", "noemail", MU_CRYPTPASS);
		if (ts != 0)
			mu->registered = ts;
		metadata_add(mu, "fake", "1");
	}
	else if (ts != 0 && ts != mu->registered)
	{
		if (backend_loaded)
		{
			slog(LG_DEBUG, "handle_setlogin(): got unexpected registration time for login %s for user %s (%lu != %lu)",
					login, u->nick,
					(unsigned long)ts,
					(unsigned long)mu->registered);
			return;
		}
		if (MOWGLI_LIST_LENGTH(&mu->logins))
			slog(LG_INFO, "handle_setlogin(): account %s with changing registration time has logins", login);
		slog(LG_DEBUG, "handle_setlogin(): changing registration time for %s from %lu to %lu",
				entity(mu)->name, (unsigned long)mu->registered,
				(unsigned long)ts);
		mu->registered = ts;
	}
	u->myuser = mu;
	u->flags &= ~UF_SOPER_PASS;
	n = mowgli_node_create();
	mowgli_node_add(u, n, &mu->logins);
	slog(LG_DEBUG, "handle_setlogin(): %s set %s logged in as %s",
			get_oper_name(si), u->nick, login);
}

void
handle_clearlogin(struct sourceinfo *si, struct user *u)
{
	mowgli_node_t *n;

	if (authservice_loaded)
	{
		wallops("Ignoring attempt from %s to clear login name for %s",
				get_oper_name(si), u->nick);
		return;
	}

	if (u->myuser == NULL)
		return;

	slog(LG_DEBUG, "handle_clearlogin(): %s cleared login for %s (%s)",
			get_oper_name(si), u->nick, entity(u->myuser)->name);
	n = mowgli_node_find(u, &u->myuser->logins);
	if (n != NULL)
	{
		mowgli_node_delete(n, &u->myuser->logins);
		mowgli_node_free(n);
	}
	u->myuser = NULL;
}

void
handle_certfp(struct sourceinfo *si, struct user *u, const char *certfp)
{
	struct myuser *mu;
	struct mycertfp *mcfp;
	struct service *svs;
	struct hook_user_login_check req;

	sfree(u->certfp);
	u->certfp = sstrdup(certfp);

	if (u->myuser != NULL)
		return;

	if ((mcfp = mycertfp_find(certfp)) == NULL)
		return;

	mu = mcfp->mu;
	svs = service_find("nickserv");
	if (svs == NULL)
		return;

	if (metadata_find(mu, "private:freeze:freezer"))
	{
		notice(svs->me->nick, u->nick, nicksvs.no_nick_ownership ? "You cannot login as \2%s\2 because the account has been frozen." : "You cannot identify to \2%s\2 because the nickname has been frozen.", entity(mu)->name);
		logcommand_user(svs, u, CMDLOG_LOGIN, "failed LOGIN to %s (frozen) via CERTFP (%s)", entity(mu)->name, certfp);
		return;
	}

	if (user_loginmaxed(mu))
	{
		notice(svs->me->nick, u->nick, "There are already \2%zu\2 sessions logged in to \2%s\2 (maximum allowed: %u).", MOWGLI_LIST_LENGTH(&mu->logins), entity(mu)->name, me.maxlogins);
		return;
	}

	req.si = si;
	req.mu = mu;
	req.allowed = true;
	hook_call_user_can_login(&req);
	if (!req.allowed)
	{
		return;
	}

	notice(svs->me->nick, u->nick, nicksvs.no_nick_ownership ? "You are now logged in as \2%s\2." : "You are now identified for \2%s\2.", entity(mu)->name);

	myuser_login(svs, u, mu, true);
	logcommand_user(svs, u, CMDLOG_LOGIN, "LOGIN via CERTFP (%s)", certfp);
}

void
myuser_login(struct service *svs, struct user *u, struct myuser *mu, bool sendaccount)
{
	char lau[BUFSIZE], lao[BUFSIZE];
	char strfbuf[BUFSIZE];
	struct metadata *md_failnum;
	struct tm *tm;
	struct mynick *mn;

	return_if_fail(svs != NULL && svs->me != NULL);
	return_if_fail(u->myuser == NULL);

	if (is_soper(mu))
		slog(LG_INFO, "SOPER: \2%s\2 as \2%s\2 (%s)", u->nick, entity(mu)->name, mu->soper->operclass->name);

	myuser_notice(svs->me->nick, mu, "%s!%s@%s has just authenticated as you (%s)", u->nick, u->user, u->vhost, entity(mu)->name);

	u->myuser = mu;
	mowgli_node_add(u, mowgli_node_create(), &mu->logins);
	u->flags &= ~UF_SOPER_PASS;

	/* check for previous login and let them know, unless they have opt'd OUT */

		if (metadata_find(mu, "private:host:actual") != NULL && metadata_find(mu, "private:showlast:optout") == NULL)
		{
			struct metadata *md_loginaddr;
			time_t ts = CURRTIME;

			md_loginaddr = metadata_find(mu, "private:host:actual");
			tm = localtime(&mu->lastlogin);
			strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

			notice(svs->me->nick, u->nick, "Last login from: \2%s\2 on %s.", md_loginaddr->value, strfbuf);

		}

	/* keep track of login address for users */
	mowgli_strlcpy(lau, u->user, BUFSIZE);
	mowgli_strlcat(lau, "@", BUFSIZE);
	mowgli_strlcat(lau, u->vhost, BUFSIZE);
	metadata_add(mu, "private:host:vhost", lau);

	/* and for opers */
	mowgli_strlcpy(lao, u->user, BUFSIZE);
	mowgli_strlcat(lao, "@", BUFSIZE);
	mowgli_strlcat(lao, u->host, BUFSIZE);
	metadata_add(mu, "private:host:actual", lao);

	/* check for failed attempts and let them know */
	if ((md_failnum = metadata_find(mu, "private:loginfail:failnum")) && (atoi(md_failnum->value) > 0))
	{
		struct metadata *md_failtime, *md_failaddr;
		time_t ts = CURRTIME;

		notice(svs->me->nick, u->nick, "\2%d\2 failed %s since last login.",
			atoi(md_failnum->value), (atoi(md_failnum->value) == 1) ? "login" : "logins");

		md_failtime = metadata_find(mu, "private:loginfail:lastfailtime");
		if (md_failtime != NULL)
			ts = atol(md_failtime->value);

		md_failaddr = metadata_find(mu, "private:loginfail:lastfailaddr");
		if (md_failaddr != NULL)
		{
			tm = localtime(&ts);
			strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

			notice(svs->me->nick, u->nick, "Last failed attempt from: \2%s\2 on %s.",
				md_failaddr->value, strfbuf);
		}

		metadata_delete(mu, "private:loginfail:failnum");	/* md_failnum now invalid */
		metadata_delete(mu, "private:loginfail:lastfailtime");
		metadata_delete(mu, "private:loginfail:lastfailaddr");
	}

	if (mu->flags & MU_WAITAUTH)
	{
		notice(svs->me->nick, u->nick, "You have \2NOT COMPLETED\2 registration verification.");
		notice(svs->me->nick, u->nick, "An email containing nickname activation instructions was sent to \2%s\2.", mu->email);
		notice(svs->me->nick, u->nick, "If you do not complete registration within one day, your nickname will expire.");
	}

	if (metadata_find(mu, "private:setpass:key"))
	{
		slog(LG_INFO, "Invalidating password reset token for %s due to successful login", entity(mu)->name);

		myuser_notice(svs->me->nick, mu, "An outstanding password reset request for your account has now "
		                                 "been invalidated.");

		metadata_delete(mu, "private:setpass:key");
		metadata_delete(mu, "private:sendpass:sender");
		metadata_delete(mu, "private:sendpass:timestamp");
	}

	mu->lastlogin = CURRTIME;
	mn = mynick_find(u->nick);
	if (mn != NULL && mn->owner == mu)
		mn->lastseen = CURRTIME;

	/* XXX: ircd_on_login supports hostmasking, we just dont have it yet. */
	/* don't allow them to join regonly chans until their
	 * email is verified */
	if (sendaccount && !(mu->flags & MU_WAITAUTH))
		ircd_on_login(u, mu, NULL);

	hook_call_user_identify(u);
}

/* this could be done with more finesse, but hey! */
static void ATHEME_FATTR_PRINTF(3, 4)
generic_notice(const char *from, const char *to, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];
	struct user *u;
	struct channel *c;

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	if (*to == '#')
	{
		c = channel_find(to);
		if (c != NULL)
			notice_channel_sts(user_find_named(from), c, buf);
	}
	else
	{
		u = user_find_named(to);
		if (u != NULL)
		{
			if (u->myuser != NULL && u->myuser->flags & MU_USE_PRIVMSG)
				msg(from, to, "%s", buf);
			else
				notice_user_sts(user_find_named(from), u, buf);
		}
	}
}

void (*notice) (const char *from, const char *target, const char *fmt, ...) = generic_notice;

/*
 * change_notify()
 *
 * Sends a change notification to a user affected by that change, provided
 * they have not disabled the messages (MU_QUIETCHG is not set).
 *
 * Inputs:
 *       - string representing source (for compatibility with notice())
 *       - struct user object to send the notice to
 *       - printf-style string containing the data to send and any args
 *
 * Outputs:
 *       - nothing
 *
 * Side Effects:
 *       - a notice is sent to a user if MU_QUIETCHG is not set.
 */
void ATHEME_FATTR_PRINTF(3, 4)
change_notify(const char *from, struct user *to, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	if (is_internal_client(to))
		return;
	if (to->myuser != NULL && to->myuser->flags & MU_QUIETCHG)
		return;

	notice_user_sts(user_find_named(from), to, buf);
}

/*
 * bad_password()
 *
 * Registers an attempt to authenticate with an incorrect password.
 *
 * Inputs:
 *       - struct sourceinfo representing what sent the bad password
 *       - struct myuser object attempt was against
 *
 * Outputs:
 *       - whether the user was killed off the network
 *
 * Side Effects:
 *       - flood counter is incremented for user
 *       - attempt is registered in metadata
 *       - opers warned if necessary
 *
 * Note:
 *       - kills are currently not done
 */
bool
bad_password(struct sourceinfo *si, struct myuser *mu)
{
	const char *mask;
	struct tm *tm;
	char numeric[21], strfbuf[BUFSIZE];
	int count;
	struct metadata *md_failnum;
	struct service *svs;

	/* If the user is already logged in, no paranoia is needed,
	 * as they could /ns set password anyway.
	 */
	if (si->smu == mu)
		return false;

	command_add_flood(si, FLOOD_MODERATE);

	mask = get_source_mask(si);

	md_failnum = metadata_find(mu, "private:loginfail:failnum");
	count = md_failnum ? atoi(md_failnum->value) : 0;
	count++;
	snprintf(numeric, sizeof numeric, "%d", count);
	metadata_add(mu, "private:loginfail:failnum", numeric);
	metadata_add(mu, "private:loginfail:lastfailaddr", mask);
	snprintf(numeric, sizeof numeric, "%lu", (unsigned long)CURRTIME);
	metadata_add(mu, "private:loginfail:lastfailtime", numeric);

	svs = si->service;
	if (svs == NULL)
		svs = service_find("nickserv");
	if (svs != NULL)
	{
		myuser_notice(svs->me->nick, mu, "\2%s\2 failed to login to \2%s\2. There %s been \2%d\2 failed login %s since your last successful login.", mask, entity(mu)->name, count == 1 ? "has" : "have", count, count == 1 ? "attempt" : "attempts");
	}

	if (is_soper(mu))
		slog(LG_INFO, "SOPER:AF: \2%s\2 FAILED Authentication as \2%s\2 (%s)", get_source_name(si), entity(mu)->name, mu->soper->operclass->name);

	if (count % 10 == 0)
	{
		time_t ts = CURRTIME;
		tm = localtime(&ts);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);
		wallops("Warning: \2%d\2 failed login attempts to \2%s\2. Last attempt received from \2%s\2 on %s.", count, entity(mu)->name, mask, strfbuf);
	}

	return false;
}

static void
sourceinfo_delete(struct sourceinfo *si)
{
	mowgli_heap_free(sourceinfo_heap, si);
}

struct sourceinfo *
sourceinfo_create(void)
{
	struct sourceinfo *out;

	if (sourceinfo_heap == NULL)
		sourceinfo_heap = sharedheap_get(sizeof(struct sourceinfo));

	out = mowgli_heap_alloc(sourceinfo_heap);
	atheme_object_init(atheme_object(out), "<sourceinfo>", (atheme_object_destructor_fn) sourceinfo_delete);

	return out;
}

void ATHEME_FATTR_PRINTF(3, 4)
command_fail(struct sourceinfo *si, enum cmd_faultcode code, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(buf, sizeof buf, fmt, args);
	va_end(args);

	if (si->v != NULL && si->v->cmd_fail)
	{
		si->v->cmd_fail(si, code, buf);
		return;
	}
	if (si->su == NULL)
		return;

	if (use_privmsg && si->smu != NULL && si->smu->flags & MU_USE_PRIVMSG)
		msg(si->service->nick, si->su->nick, "%s", buf);
	else
		notice_user_sts(si->service->me, si->su, buf);
}

void ATHEME_FATTR_PRINTF(2, 3)
command_success_nodata(struct sourceinfo *si, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];
	char *p, *q;
	char space[] = " ";

	if (si->output_limit && si->output_count > si->output_limit)
		return;
	si->output_count++;

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	if (si->v != NULL && si->v->cmd_success_nodata)
	{
		si->v->cmd_success_nodata(si, buf);
		return;
	}
	if (si->su == NULL)
		return;

	if (si->output_limit && si->output_count > si->output_limit)
	{
		notice(si->service->nick, si->su->nick, _("Output limit (%u) exceeded, halting output"), si->output_limit);
		return;
	}

	p = buf;
	do
	{
		q = strchr(p, '\n');
		if (q != NULL)
		{
			*q++ = '\0';
			if (*q == '\0')
				return; /* ending with \n */
		}
		if (*p == '\0')
			p = space; /* replace empty lines with a space */
		if (use_privmsg && si->smu != NULL && si->smu->flags & MU_USE_PRIVMSG)
			msg(si->service->nick, si->su->nick, "%s", p);
		else
			notice_user_sts(si->service->me, si->su, p);
		p = q;
	} while (p != NULL);
}

void ATHEME_FATTR_PRINTF(3, 4)
command_success_string(struct sourceinfo *si, const char *result, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	if (si->v != NULL && si->v->cmd_success_string)
	{
		si->v->cmd_success_string(si, result, buf);
		return;
	}
	if (si->su == NULL)
		return;

	if (use_privmsg && si->smu != NULL && si->smu->flags & MU_USE_PRIVMSG)
		msg(si->service->nick, si->su->nick, "%s", buf);
	else
		notice_user_sts(si->service->me, si->su, buf);
}

static void
command_table_cb(const char *line, void *data)
{
	command_success_nodata(data, "%s", line);
}

void
command_success_table(struct sourceinfo *si, struct atheme_table *table)
{
	if (si->v != NULL && si->v->cmd_success_table)
	{
		si->v->cmd_success_table(si, table);
		return;
	}

	table_render(table, command_table_cb, si);
}

const char *
get_source_name(struct sourceinfo *si)
{
	static char result[NICKLEN + 1 + NICKLEN + 1 + 10];

	if (si->v != NULL && si->v->get_source_name != NULL)
		return si->v->get_source_name(si);

	if (si->su != NULL)
	{
		if (si->smu && !irccasecmp(si->su->nick, entity(si->smu)->name))
			snprintf(result, sizeof result, "%s", si->su->nick);
		else
			snprintf(result, sizeof result, "%s (%s)", si->su->nick,
					si->smu ? entity(si->smu)->name : "");
	}
	else if (si->s != NULL)
		snprintf(result, sizeof result, "%s", si->s->name);
	else if (si->v != NULL)
		snprintf(result, sizeof result, "<%s>%s", si->v->description,
				si->smu ? entity(si->smu)->name : "");
	else
		mowgli_strlcpy(result, "???", sizeof result);

	return result;
}

const char *
get_source_mask(struct sourceinfo *si)
{
	static char result[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1 + 10];

	if (si->v != NULL && si->v->get_source_mask != NULL)
		return si->v->get_source_mask(si);

	if (si->su != NULL)
		snprintf(result, sizeof result, "%s!%s@%s", si->su->nick,
				si->su->user, si->su->vhost);
	else if (si->s != NULL)
		snprintf(result, sizeof result, "%s", si->s->name);
	else if (si->v != NULL)
		snprintf(result, sizeof result, "<%s>%s", si->v->description,
				si->smu ? entity(si->smu)->name : "");
	else
		mowgli_strlcpy(result, "???", sizeof result);

	return result;
}

const char *
get_oper_name(struct sourceinfo *si)
{
	static char result[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1 + NICKLEN + 10];

	if (si->v != NULL && si->v->get_oper_name != NULL)
		return si->v->get_oper_name(si);

	if (si->su != NULL)
	{
		if (si->smu == NULL)
			snprintf(result, sizeof result, "%s!%s@%s{%s}", si->su->nick,
					si->su->user, si->su->vhost,
					si->su->server->name);
		else if (!irccasecmp(si->su->nick, entity(si->smu)->name))
			snprintf(result, sizeof result, "%s", si->su->nick);
		else
			snprintf(result, sizeof result, "%s (%s)", si->su->nick,
					si->smu ? entity(si->smu)->name : "");
	}
	else if (si->s != NULL)
		snprintf(result, sizeof result, "%s", si->s->name);
	else if (si->v != NULL)
		snprintf(result, sizeof result, "<%s>%s", si->v->description,
				si->smu ? entity(si->smu)->name : "");
	else
		mowgli_strlcpy(result, "???", sizeof result);

	return result;
}

const char *
get_storage_oper_name(struct sourceinfo *si)
{
	static char result[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1 + NICKLEN + 10];

	if (si->v != NULL && si->v->get_storage_oper_name != NULL)
		return si->v->get_storage_oper_name(si);

	if (si->smu != NULL)
		snprintf(result, sizeof result, "%s", entity(si->smu)->name);
	else if (si->su != NULL)
		snprintf(result, sizeof result, "%s!%s@%s{%s}", si->su->nick,
				si->su->user, si->su->vhost,
				si->su->server->name);
	else if (si->s != NULL)
		snprintf(result, sizeof result, "%s", si->s->name);
	else if (si->v != NULL)
		snprintf(result, sizeof result, "<%s>", si->v->description);
	else
		mowgli_strlcpy(result, "???", sizeof result);

	return result;
}

const char *
get_source_security_label(struct sourceinfo *si)
{
	static char result[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1 + NICKLEN + 1 + HOSTLEN + 1 + 10];
	const struct soper *soper;
	const struct operclass *operclass = NULL;

	mowgli_strlcpy(result, get_storage_oper_name(si), sizeof result);

	soper = get_sourceinfo_soper(si);
	if (soper != NULL)
	{
		mowgli_strlcat(result, "/", sizeof result);
		/* If the soper is attached to an entity, then soper->name
		 * is null. Use soper->myuser->ent.name instead. --mr_flea
		 */
		mowgli_strlcat(result, soper->name ? soper->name : soper->myuser->ent.name,
			sizeof result);

		operclass = soper->operclass;
	}

	if (operclass == NULL)
		operclass = get_sourceinfo_operclass(si);

	mowgli_strlcat(result, "/", sizeof result);
	mowgli_strlcat(result, operclass != NULL ? operclass->name : "unauthenticated", sizeof result);

	return result;
}

void ATHEME_FATTR_PRINTF(1, 2)
wallops(const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	if (config_options.silent)
		return;

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	if (me.me != NULL && me.connected)
		wallops_sts(buf);
	else
		slog(LG_ERROR, "wallops(): unable to send: %s", buf);
}

void ATHEME_FATTR_PRINTF(1, 2)
verbose_wallops(const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	if (config_options.silent || !config_options.verbose_wallops)
		return;

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	if (me.me != NULL && me.connected)
		wallops_sts(buf);
	else
		slog(LG_ERROR, "verbose_wallops(): unable to send: %s", buf);
}

/* Check validity of a vhost against both general and protocol-specific rules.
 * Returns true if it is ok, false if not; command_fail() will have been called
 * as well.
 */
bool
check_vhost_validity(struct sourceinfo *si, const char *host)
{
	const char *p;

	/* Never ever allow @!?* as they have special meaning in all ircds */
	/* Empty, space anywhere and colon at the start break the protocol */
	/* Also disallow ASCII 1-31 and "' as no sane IRCd allows them in hosts */
	if (strchr(host, '@') || strchr(host, '!') || strchr(host, '?') ||
			strchr(host, '*') || strchr(host, ' ') || strchr(host, '\'') ||
			strchr(host, '"') || *host == ':' || *host == '\0' ||
			has_ctrl_chars(host))
	{
		command_fail(si, fault_badparams, _("The vhost provided contains invalid characters."));
		return false;
	}
	if (strlen(host) > HOSTLEN)
	{
		command_fail(si, fault_badparams, _("The vhost provided is too long."));
		return false;
	}
	p = strrchr(host, '/');
	if (p != NULL && isdigit((unsigned char)p[1]))
	{
		command_fail(si, fault_badparams, _("The vhost provided looks like a CIDR mask."));
		return false;
	}
	if (!is_valid_host(host))
	{
		/* This can be stuff like missing dots too. */
		command_fail(si, fault_badparams, _("The vhost provided is invalid."));
		return false;
	}
	return true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
