/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains client interaction routines.
 *
 * $Id: services.c 5716 2006-07-04 04:19:46Z gxti $
 */

#include "atheme.h"

extern list_t services[HASHSIZE];

#define MAX_BUF 256

/* ban wrapper for cmode, returns number of bans added (0 or 1) */
int ban(char *sender, char *channel, user_t *user)
{
	char mask[MAX_BUF];
	char modemask[MAX_BUF];
	channel_t *c = channel_find(channel);
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

	mode_sts(sender, channel, modemask);
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

		/* XXX doesn't do CIDR bans */
		if (!match(cb->mask, hostbuf) || !match(cb->mask, hostbuf2) || !match(cb->mask, hostbuf3))
		{
			snprintf(change, sizeof change, "-%c %s", e, cb->mask);
			mode_sts(source->nick, chan->name, change);
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

	u = user_find_named(nick);
	if (!u)
		return;
	c = channel_find(chan);
	if (c == NULL)
	{
		mc = mychan_find(chan);
		c = channel_add(chan, chansvs.changets && mc != NULL && mc->registered > 0 ? mc->registered : CURRTIME);
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

/* bring on the services clients */
void services_init(void)
{
	node_t *n;
	uint32_t i;
	service_t *svs;

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, services[i].head)
		{
			svs = n->data;

			if (ircd->uses_uid && svs->me->uid[0] == '\0')
				user_changeuid(svs->me, svs->uid);
			else if (!ircd->uses_uid && svs->me->uid[0] != '\0')
				user_changeuid(svs->me, NULL);
			introduce_nick(svs->name, svs->user, svs->host, svs->real, svs->uid);
			/* we'll join config_options.chan later -- jilles */
		}
	}
}

void joinall(char *name)
{
	node_t *n;
	uint32_t i;
	service_t *svs;

	if (name == NULL)
		return;
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, services[i].head)
		{
			svs = n->data;
			join(name, svs->name);
		}
	}
}

void partall(char *name)
{
	node_t *n;
	uint32_t i;
	service_t *svs;
	mychan_t *mc;

	if (name == NULL)
		return;
	mc = mychan_find(name);
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, services[i].head)
		{
			svs = n->data;
			if (svs == chansvs.me && mc != NULL && config_options.join_chans)
				continue;
			/* Do not cache this channel_find(), the
			 * channel may disappear under our feet
			 * -- jilles */
			if (chanuser_find(channel_find(name), svs->me))
				part(name, svs->name);
		}
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
	introduce_nick(u->nick, u->user, u->host, svs->real, u->uid);
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

	if (MC_VERBOSE & mychan->flags)
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
	myuser_t *mu;
    node_t *n;

	if (u == NULL)
		return;

	if (me.loglevel & LG_DEBUG && runflags & RF_LIVE)
		notice(globsvs.nick, u->nick, "Services are presently running in debug mode, attached to a console. You should take extra caution when utilizing your services passwords.");

	/* Only do the following checks for nickserv, not userserv -- jilles */
	if (nicksvs.me == NULL)
		return;

	/* They're logged in, don't send them spam -- jilles */
	if (u->myuser)
		u->flags |= UF_SEENINFO;

	/* Also don't send it if they came back from a split -- jilles */
	if (!(u->server->flags & SF_EOB))
		u->flags |= UF_SEENINFO;

    /* If the user recently completed SASL login, omit -- gxti */
    if(*u->uid)
    {
        LIST_FOREACH(n, saslsvs.pending.head)
        {
            if(!strcmp((char*)n->data, u->uid))
                return;
        }
    }

	if (!(mu = myuser_find(u->nick)))
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

	if (u->myuser == mu)
		return;

	/* If we're MU_SASL, then this user has just identified by SASL
	 * (we just don't know it yet). So, we bypass the complaint below.
	 *
	 * This is not a major concern, as MU_SASL is a hack intended to bypass
	 * NickServ anyway. The u->myuser relationship isn't set up until later.
	 *
	 *   - nenolod
	 */
	if (mu->flags & MU_SASL)
	{
		mu->flags &= ~MU_SASL;
		return;
	}

	notice(nicksvs.nick, u->nick, "This nickname is registered. Please choose a different nickname, or identify via \2/%s%s identify <password>\2.",
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

	hook_call_event("user_burstlogin", u);

	mu = myuser_find(login);
	if (mu == NULL)
	{
		/* account dropped during split...
		 * if we have an authentication service, log them out */
		slog(LG_DEBUG, "handle_burstlogin(): got nonexistent login %s for user %s", login, u->nick);
		if (authservice_loaded)
			ircd_on_logout(u->nick, login, NULL);
		return;
	}
	if (u->myuser != NULL)	/* already logged in, hmm */
		return;
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
	char *str = translation_get(fmt);

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, str, args);
	va_end(args);

	if (config_options.use_privmsg)
		msg(from, to, "%s", buf);
	else
		notice_sts(from, to, "%s", buf);
}

void verbose_wallops(char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	if (config_options.verbose_wallops != TRUE)
		return;

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	wallops("%s", buf);
}
