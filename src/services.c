/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains client interaction routines.
 *
 * $Id: services.c 3289 2005-10-30 20:37:14Z jilles $
 */

#include "atheme.h"

extern list_t services[HASHSIZE];

#define MAX_BUF 256

/* ban wrapper for cmode */
void ban(char *sender, char *channel, user_t *user)
{
	char mask[MAX_BUF];
	char modemask[MAX_BUF];
	channel_t *c = channel_find(channel);
	chanban_t *cb;

	if (!c)
		return;

	snprintf(mask, MAX_BUF, "*!*@%s", user->vhost);
	mask[MAX_BUF - 1] = '\0';

	snprintf(modemask, MAX_BUF, "+b %s", mask);
	modemask[MAX_BUF - 1] = '\0';

	cb = chanban_find(c, mask);

	if (cb != NULL)
		return;

	chanban_add(c, mask);

	mode_sts(sender, channel, modemask);
}

/* join a channel, creating it if necessary */
void join(char *chan, char *nick)
{
	channel_t *c;
	user_t *u;
	chanuser_t *cu;
	boolean_t isnew = FALSE;
	mychan_t *mc;

	u = user_find(nick);
	if (!u)
		return;
	c = channel_find(chan);
	if (c == NULL)
	{
		c = channel_add(chan, CURRTIME);
		c->modes |= CMODE_NOEXT | CMODE_TOPIC;
		mc = mychan_find(c->name);
		if (mc != NULL)
			check_modes(mc, FALSE);
		isnew = TRUE;
	}
	else if ((cu = chanuser_find(c, u)))
	{
		slog(LG_DEBUG, "join(): i'm already in `%s'", c->name);
		return;
	}
	cu = chanuser_add(c, nick);
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
		if (LIST_LENGTH(&c->members) > 1)
			join_sts(c, u, 0, channel_modes(c, TRUE));
		else
		{
			/* channel will have been destroyed... */
			/* XXX ban desync between services and ircd */
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

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	if (MC_VERBOSE & mychan->flags)
		notice(chansvs.nick, mychan->name, buf);
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
	metadata_t *md;

	if (me.loglevel & LG_DEBUG && runflags & RF_LIVE)
		notice(globsvs.nick, u->nick, "Services are presently running in debug mode, attached to a console. " "You should take extra caution when utilizing your services passwords.");

	/* They're logged in, don't send them spam -- jilles */
	if (u->myuser)
		u->flags |= UF_SEENINFO;

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

	if ((mu->flags & MU_ALIAS) && (md = metadata_find(mu, METADATA_USER, "private:alias:parent")) && u->myuser == myuser_find(md->value))
		return;

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

	mu = myuser_find(login);
	if (mu == NULL)
	{
		/* account dropped during split... log them out */
		slog(LG_DEBUG, "handle_burstlogin(): got nonexistent login %s for user %s", login, u->nick);
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

struct command_ *cmd_find(char *svs, char *origin, char *command, struct command_ table[])
{
	user_t *u = user_find(origin);
	struct command_ *c;

	for (c = table; c->name; c++)
	{
		if (!strcasecmp(command, c->name))
		{
			/* no special access required, so go ahead... */
			if (c->access == AC_NONE)
				return c;

			/* sra? */
			if ((c->access == AC_SRA) && (is_sra(u->myuser)))
				return c;

			/* ircop? */
			if ((c->access == AC_IRCOP) && (is_sra(u->myuser) || (is_ircop(u))))
				return c;

			/* otherwise... */
			else
			{
				notice(svs, origin, "You are not authorized to perform this operation.");
				snoop("DENIED CMD: \2%s\2 used %s", origin, command);
				return NULL;
			}
		}
	}

	/* it's a command we don't understand */
	notice(svs, origin, "Invalid command. Please use \2HELP\2 for help.");
	return NULL;
}
