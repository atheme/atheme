/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains client interaction routines.
 *
 * $Id: services.c 7973 2007-03-23 21:45:12Z jilles $
 */

#include "atheme.h"
#include "pmodule.h"

extern dictionary_tree_t *services;
int authservice_loaded = 0;
int use_myuser_access = 0;
int use_svsignore = 0;

#define MAX_BUF 256

/* ban wrapper for cmode, returns number of bans added (0 or 1) */
int ban(user_t *sender, channel_t *c, user_t *user)
{
	char mask[MAX_BUF];
	char modemask[MAX_BUF];
	chanban_t *cb;

	if (!c)
		return 0;

	snprintf(mask, MAX_BUF, "*!*@%s", user->vhost);
	mask[MAX_BUF - 1] = '\0';

	snprintf(modemask, MAX_BUF, "+b %s", mask);
	modemask[MAX_BUF - 1] = '\0';

	cb = chanban_find(c, mask, 'b');

	if (cb != NULL)
		return 0;

	chanban_add(c, mask, 'b');

	mode_sts(sender->nick, c, modemask);
	return 1;
}

/* returns number of exceptions removed -- jilles */
int remove_ban_exceptions(user_t *source, channel_t *chan, user_t *target)
{
	char change[MAX_BUF];
	char e;
	char hostbuf[BUFSIZE], hostbuf2[BUFSIZE], hostbuf3[BUFSIZE];
	int count = 0;
	node_t *n, *tn;

	e = ircd->except_mchar;
	if (e == '\0')
		return 0;
	if (source == NULL || chan == NULL || target == NULL)
		return 0;

	snprintf(hostbuf, BUFSIZE, "%s!%s@%s", target->nick, target->user, target->host);
	snprintf(hostbuf2, BUFSIZE, "%s!%s@%s", target->nick, target->user, target->vhost);
	/* will be nick!user@ if ip unknown, doesn't matter */
	snprintf(hostbuf3, BUFSIZE, "%s!%s@%s", target->nick, target->user, target->ip);
	LIST_FOREACH_SAFE(n, tn, chan->bans.head)
	{
		chanban_t *cb = n->data;

		if (cb->type != e)
			continue;

		if (!match(cb->mask, hostbuf) || !match(cb->mask, hostbuf2) || !match(cb->mask, hostbuf3) || !match_cidr(cb->mask, hostbuf3))
		{
			snprintf(change, sizeof change, "-%c %s", e, cb->mask);
			mode_sts(source->nick, chan, change);
			chanban_delete(cb);
			count++;
		}
	}
	return count;
}

/* join a channel, creating it if necessary */
void join(char *chan, char *nick)
{
	channel_t *c;
	user_t *u;
	chanuser_t *cu;
	boolean_t isnew = FALSE;
	mychan_t *mc;
	metadata_t *md;
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
			md = metadata_find(mc, METADATA_CHANNEL, "private:channelts");
			if (md != NULL)
				ts = atol(md->value);
			if (ts == 0)
				ts = CURRTIME;
		}
		else
			ts = CURRTIME;
		c = channel_add(chan, ts);
		c->modes |= CMODE_NOEXT | CMODE_TOPIC;
		if (mc != NULL)
			check_modes(mc, FALSE);
		isnew = TRUE;
	}
	else if ((cu = chanuser_find(c, u)))
	{
		slog(LG_DEBUG, "join(): i'm already in `%s'", c->name);
		return;
	}
	cu = chanuser_add(c, CLIENT_NAME(u));
	cu->modes |= CMODE_OP;
	join_sts(c, u, isnew, channel_modes(c, TRUE));
}

/* part a channel */
void part(char *chan, char *nick)
{
	channel_t *c = channel_find(chan);
	user_t *u = user_find_named(nick);

	if (!u || !c)
		return;
	if (!chanuser_find(c, u))
		return;
	part_sts(c, u);
	chanuser_delete(c, u);
}

void services_init(void)
{
	service_t *svs;
	dictionary_iteration_state_t state;

	DICTIONARY_FOREACH(svs, &state, services)
	{
		if (ircd->uses_uid && svs->me->uid[0] == '\0')
			user_changeuid(svs->me, svs->uid);
		else if (!ircd->uses_uid && svs->me->uid[0] != '\0')
			user_changeuid(svs->me, NULL);
		introduce_nick(svs->me);
	}
}

void joinall(char *name)
{
	service_t *svs;
	dictionary_iteration_state_t state;

	if (name == NULL)
		return;

	DICTIONARY_FOREACH(svs, &state, services)
	{
		join(name, svs->name);
	}
}

void partall(char *name)
{
	dictionary_iteration_state_t state;
	service_t *svs;
	mychan_t *mc;

	if (name == NULL)
		return;
	mc = mychan_find(name);
	DICTIONARY_FOREACH(svs, &state, services)
	{
		if (svs == chansvs.me && mc != NULL && config_options.join_chans)
			continue;
		/* Do not cache this channel_find(), the
		 * channel may disappear under our feet
		 * -- jilles */
		if (chanuser_find(channel_find(name), svs->me))
			part(name, svs->name);
	}
}

/* reintroduce a service e.g. after it's been killed -- jilles */
void reintroduce_user(user_t *u)
{
	node_t *n;
	channel_t *c;
	/*char chname[256];*/
	service_t *svs;

	svs = find_service(u->nick);
	if (svs == NULL)
	{
		slog(LG_DEBUG, "tried to reintroduce_user non-service %s", u->nick);
		return;
	}
	introduce_nick(u);
	LIST_FOREACH(n, u->channels.head)
	{
		c = ((chanuser_t *)n->data)->chan;
		if (LIST_LENGTH(&c->members) > 1 || c->modes & ircd->perm_mode)
			join_sts(c, u, 0, channel_modes(c, TRUE));
		else
		{
			/* channel will have been destroyed... */
			/* XXX resend the bans instead of destroying them? */
			chanban_clear(c);
			join_sts(c, u, 1, channel_modes(c, TRUE));
			if (c->topic != NULL)
				topic_sts(c, c->topic_setter, c->topicts, 0, c->topic);
#if 0
			strlcpy(chname, c->name, sizeof chname);
			chanuser_delete(c, u);
			join(chname, u->nick);
#endif
		}
	}
}

void verbose(mychan_t *mychan, char *fmt, ...)
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

void snoop(char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	if (!config_options.chan)
		return;

	if (me.bursting)
		return;

	if (!channel_find(config_options.chan))
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	msg(opersvs.nick, config_options.chan, "%s", buf);
}

/* protocol wrapper for nickchange/nick burst */
void handle_nickchange(user_t *u)
{
	mynick_t *mn;

	if (u == NULL)
		return;

	if ((log_force || (me.loglevel & (LG_DEBUG | LG_RAWDATA))) &&
			runflags & RF_LIVE)
		notice(globsvs.nick, u->nick, "Services are presently running in debug mode, attached to a console. You should take extra caution when utilizing your services passwords.");

	/* Only do the following checks if nicks are considered owned -- jilles */
	if (nicksvs.me == NULL || nicksvs.no_nick_ownership)
		return;

	/* They're logged in, don't send them spam -- jilles */
	if (u->myuser)
		u->flags |= UF_SEENINFO;

	/* Also don't send it if they came back from a split -- jilles */
	if (!(u->server->flags & SF_EOB))
		u->flags |= UF_SEENINFO;

	if (!(mn = mynick_find(u->nick)))
	{
		if (!nicksvs.spam)
			return;

		if (!(u->flags & UF_SEENINFO))
		{
			notice(nicksvs.nick, u->nick, "Welcome to %s, %s! Here on %s, we provide services to enable the "
			       "registration of nicknames and channels! For details, type \2/%s%s help\2 and \2/%s%s help\2.",
			       me.netname, u->nick, me.netname, (ircd->uses_rcommand == FALSE) ? "msg " : "", nicksvs.disp, (ircd->uses_rcommand == FALSE) ? "msg " : "", chansvs.disp);

			u->flags |= UF_SEENINFO;
		}

		return;
	}

	if (u->myuser == mn->owner)
	{
		mn->lastseen = CURRTIME;
		return;
	}

	/* OpenServices: is user on access list? -nenolod */
	if (myuser_access_verify(u, mn->owner))
	{
		mn->lastseen = CURRTIME;
		return;
	}

	notice(nicksvs.nick, u->nick, _("This nickname is registered. Please choose a different nickname, or identify via \2/%s%s identify <password>\2."),
	       (ircd->uses_rcommand == FALSE) ? "msg " : "", nicksvs.disp);
}

/* User u is bursted as being logged in to login
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
void handle_burstlogin(user_t *u, char *login)
{
	myuser_t *mu;
	node_t *n;

	/* don't allow alias nicks here -- jilles */
	mu = myuser_find(login);
	if (mu == NULL)
	{
		/* account dropped during split...
		 * if we have an authentication service, log them out */
		slog(LG_DEBUG, "handle_burstlogin(): got nonexistent login %s for user %s", login, u->nick);
		if (authservice_loaded)
		{
			notice(nicksvs.nick ? nicksvs.nick : me.name, u->nick, _("Account %s dropped, forcing logout"), login);
			ircd_on_logout(u->nick, login, NULL);
		}
		return;
	}
	if (u->myuser != NULL)	/* already logged in, hmm */
		return;
	if (mu->flags & MU_NOBURSTLOGIN && authservice_loaded)
	{
		/* no splits for this account, this bursted login cannot
		 * be legit...
		 * if we have an authentication service, log them out */
		slog(LG_INFO, "handle_burstlogin(): got illegit login %s for user %s", login, u->nick);
		notice(nicksvs.nick ? nicksvs.nick : me.name, u->nick, _("Login to account %s seems invalid, forcing logout"), login);
		ircd_on_logout(u->nick, login, NULL);
		return;
	}
	u->myuser = mu;
	n = node_create();
	node_add(u, n, &mu->logins);
	slog(LG_DEBUG, "handle_burstlogin(): automatically identified %s as %s", u->nick, login);
}

/* this could be done with more finesse, but hey! */
void notice(char *from, char *to, char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];
	const char *str = translation_get(fmt);
	user_t *u;
	channel_t *c;

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, str, args);
	va_end(args);

	if (config_options.use_privmsg)
		msg(from, to, "%s", buf);
	else
	{
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
				notice_user_sts(user_find_named(from), u, buf);
		}
	}
}

void command_fail(sourceinfo_t *si, faultcode_t code, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];
	const char *str = translation_get(fmt);

	va_start(args, fmt);
	vsnprintf(buf, sizeof buf, str, args);
	va_end(args);

	if (si->su == NULL)
	{
		if (si->v != NULL && si->v->cmd_fail)
			si->v->cmd_fail(si, code, buf);
		return;
	}

	if (config_options.use_privmsg)
		msg(si->service->name, si->su->nick, "%s", buf);
	else
		notice_user_sts(si->service->me, si->su, buf);
}

void command_success_nodata(sourceinfo_t *si, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];
	const char *str = translation_get(fmt);

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, str, args);
	va_end(args);

	if (si->su == NULL)
	{
		if (si->v != NULL && si->v->cmd_fail)
			si->v->cmd_success_nodata(si, buf);
		return;
	}

	if (config_options.use_privmsg)
		msg(si->service->name, si->su->nick, "%s", buf);
	else
		notice_user_sts(si->service->me, si->su, buf);
}

void command_success_string(sourceinfo_t *si, const char *result, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];
	const char *str = translation_get(fmt);

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, str, args);
	va_end(args);

	if (si->su == NULL)
	{
		if (si->v != NULL && si->v->cmd_fail)
			si->v->cmd_success_string(si, result, buf);
		return;
	}

	if (config_options.use_privmsg)
		msg(si->service->name, si->su->nick, "%s", buf);
	else
		notice_user_sts(si->service->me, si->su, buf);
}

#if 0
/* TBD */
void command_success_struct(sourceinfo_t *si, char *name, char *value)
{
	const char *name1 = translation_get(name);

	if (config_options.use_privmsg)
		msg(si->service->name, si->su->nick, "%-16s: %s", name, value);
	else
		notice_user_sts(si->service->me, si->su, buf);
}

void command_success_table_init(sourceinfo_t *si, int count, ...)
{
	va_list args;
	char *name, *name1;
	char buf[BUFSIZE];
	int i;
	int size;

	va_start(args, count);
	buf[0] = '\0';
	for (i = 0; i < count; i++)
	{
		name = va_arg(args, char *);
		name1 = translation_get(name);
		size = va_arg(args, int);
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				"%-*s ", size, name1);
	}
	va_end(args);
	if (config_options.use_privmsg)
		msg(si->service->name, si->su->nick, "%s", buf);
	else
		notice_user_sts(si->service->me, si->su, buf);
}
#endif

const char *get_source_name(sourceinfo_t *si)
{
	static char result[NICKLEN+NICKLEN+10];

	if (si->su != NULL)
	{
		if (si->smu && !irccasecmp(si->su->nick, si->smu->name))
			snprintf(result, sizeof result, "%s", si->su->nick);
		else
			snprintf(result, sizeof result, "%s(%s)", si->su->nick,
					si->smu ? si->smu->name : "");
	}
	else if (si->s != NULL)
		snprintf(result, sizeof result, "%s", si->s->name);
	else
	{
		snprintf(result, sizeof result, "<%s>%s", si->v->description,
				si->smu ? si->smu->name : "");
	}
	return result;
}

const char *get_source_mask(sourceinfo_t *si)
{
	static char result[NICKLEN+USERLEN+HOSTLEN+10];

	if (si->su != NULL)
	{
		snprintf(result, sizeof result, "%s!%s@%s", si->su->nick,
				si->su->user, si->su->vhost);
	}
	else if (si->s != NULL)
		snprintf(result, sizeof result, "%s", si->s->name);
	else
	{
		snprintf(result, sizeof result, "<%s>%s", si->v->description,
				si->smu ? si->smu->name : "");
	}
	return result;
}

const char *get_oper_name(sourceinfo_t *si)
{
	static char result[NICKLEN+USERLEN+HOSTLEN+NICKLEN+10];

	if (si->su != NULL)
	{
		if (si->smu == NULL)
			snprintf(result, sizeof result, "%s!%s@%s{%s}", si->su->nick,
					si->su->user, si->su->vhost,
					si->su->server->name);
		else if (!irccasecmp(si->su->nick, si->smu->name))
			snprintf(result, sizeof result, "%s", si->su->nick);
		else
			snprintf(result, sizeof result, "%s(%s)", si->su->nick,
					si->smu ? si->smu->name : "");
	}
	else if (si->s != NULL)
		snprintf(result, sizeof result, "%s", si->s->name);
	else
	{
		snprintf(result, sizeof result, "<%s>%s", si->v->description,
				si->smu ? si->smu->name : "");
	}
	return result;
}

void wallops(char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	if (config_options.silent)
		return;

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, translation_get(fmt), args);
	va_end(args);

	if (me.me != NULL && me.connected)
		wallops_sts(buf);
	else
		slog(LG_ERROR, "wallops(): unable to send: %s", buf);
}

void verbose_wallops(char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	if (config_options.silent || !config_options.verbose_wallops)
		return;

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, translation_get(fmt), args);
	va_end(args);

	if (me.me != NULL && me.connected)
		wallops_sts(buf);
	else
		slog(LG_ERROR, "verbose_wallops(): unable to send: %s", buf);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
