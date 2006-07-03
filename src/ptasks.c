/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol tasks, such as handle_stats().
 *
 * $Id: ptasks.c 5708 2006-07-03 23:37:53Z jilles $
 */

#include "atheme.h"

void handle_info(user_t *u)
{
	uint8_t i;

	if (u == NULL)
		return;
	if (floodcheck(u, NULL))
		return;

	for (i = 0; infotext[i]; i++)
		numeric_sts(me.name, 371, u->nick, ":%s", infotext[i]);

	numeric_sts(me.name, 374, u->nick, ":End of /INFO list");
}

void handle_version(user_t *u)
{

	if (u == NULL)
		return;
	if (floodcheck(u, NULL))
		return;

	numeric_sts(me.name, 351, u->nick, ":atheme-%s. %s %s%s%s%s%s%s%s%s%s [%s]",
		    version, me.name, 
		    (match_mapping) ? "A" : "",
		    (me.loglevel & LG_DEBUG) ? "d" : "",
		    (me.auth) ? "e" : "",
		    (config_options.flood_msgs) ? "F" : "",
		    (config_options.leave_chans) ? "l" : "", 
		    (config_options.join_chans) ? "j" : "", 
		    (!match_mapping) ? "R" : "",
		    (config_options.raw) ? "r" : "",
		    (runflags & RF_LIVE) ? "n" : "",
		    ircd->ircdname);
	numeric_sts(me.name, 351, u->nick, ":Compile time: %s, build-id %s, build %s", creation, revision, generation);
}

void handle_admin(user_t *u)
{

	if (u == NULL)
		return;

	if (floodcheck(u, NULL))
		return;

	numeric_sts(me.name, 256, u->nick, ":Administrative info about %s", me.name);
	numeric_sts(me.name, 257, u->nick, ":%s", me.adminname);
	numeric_sts(me.name, 258, u->nick, ":Atheme IRC Services (atheme-%s)", version);
	numeric_sts(me.name, 259, u->nick, ":<%s>", me.adminemail);
}

void handle_stats(user_t *u, char req)
{
	kline_t *k;
	node_t *n;
	uplink_t *uplink;
	soper_t *soper;
	int i;

	if (floodcheck(u, NULL))
		return;
	logcommand(NULL, u, CMDLOG_GET, "STATS %c", req);

	switch (req)
	{
	  case 'C':
	  case 'c':
		  if (!has_priv(u, PRIV_SERVER_AUSPEX))
			  break;

		  LIST_FOREACH(n, uplinks.head)
		  {
			  uplink = (uplink_t *)n->data;
			  numeric_sts(me.name, 213, u->nick, "C *@127.0.0.1 A %s %d uplink", uplink->name, uplink->port);
		  }
		  break;

	  case 'E':
	  case 'e':
		  if (!has_priv(u, PRIV_SERVER_AUSPEX))
			  break;

		  numeric_sts(me.name, 249, u->nick, "E :Last event to run: %s", last_event_ran);

		  numeric_sts(me.name, 249, u->nick, "E :%-28s %s", "Operation", "Next Execution");
		  for (i = 0; i < MAX_EVENTS; i++)
		  {
			  if (event_table[i].active)
				  numeric_sts(me.name, 249, u->nick, "E :%-28s %4d seconds (%d)", event_table[i].name, event_table[i].when - CURRTIME, event_table[i].frequency);
		  }

		  break;

	  case 'H':
	  case 'h':
		  if (!has_priv(u, PRIV_SERVER_AUSPEX))
			  break;

		  LIST_FOREACH(n, uplinks.head)
		  {
			  uplink = (uplink_t *)n->data;
			  numeric_sts(me.name, 244, u->nick, "H * * %s", uplink->name);
		  }
		  break;

	  case 'I':
	  case 'i':
		  numeric_sts(me.name, 215, u->nick, "I * * *@%s 0 nonopered", me.name);
		  break;

	  case 'K':
	  case 'k':
		  if (!has_priv(u, PRIV_AKILL))
			  break;

		  LIST_FOREACH(n, klnlist.head)
		  {
			  k = (kline_t *)n->data;

			  numeric_sts(me.name, 216, u->nick, "K %s * %s :%s", k->host, k->user, k->reason);
		  }

		  break;

	  case 'o':
	  case 'O':
		  if (!has_priv(u, PRIV_VIEWPRIVS))
			  break;

		  LIST_FOREACH(n, soperlist.head)
		  {
			  soper = n->data;

			  numeric_sts(me.name, 243, u->nick, "O *@* * %s %s %s",
					  soper->myuser ? soper->myuser->name : soper->name,
					  soper->operclass ? soper->operclass->name : "*", "-1");
		  }
		  break;

	  case 'T':
	  case 't':
		  if (!has_priv(u, PRIV_SERVER_AUSPEX))
			  break;

		  numeric_sts(me.name, 249, u->nick, "T :event      %7d", claro_state.event);
		  numeric_sts(me.name, 249, u->nick, "T :node       %7d", claro_state.node);
		  numeric_sts(me.name, 249, u->nick, "T :connection %7d", connection_count());
		  numeric_sts(me.name, 249, u->nick, "T :operclass  %7d", cnt.operclass);
		  numeric_sts(me.name, 249, u->nick, "T :soper      %7d", cnt.soper);
		  numeric_sts(me.name, 249, u->nick, "T :tld        %7d", cnt.tld);
		  numeric_sts(me.name, 249, u->nick, "T :kline      %7d", cnt.kline);
		  numeric_sts(me.name, 249, u->nick, "T :server     %7d", cnt.server);
		  numeric_sts(me.name, 249, u->nick, "T :user       %7d", cnt.user);
		  numeric_sts(me.name, 249, u->nick, "T :chan       %7d", cnt.chan);
		  numeric_sts(me.name, 249, u->nick, "T :myuser     %7d", cnt.myuser);
		  numeric_sts(me.name, 249, u->nick, "T :mychan     %7d", cnt.mychan);
		  numeric_sts(me.name, 249, u->nick, "T :chanacs    %7d", cnt.chanacs);

		  numeric_sts(me.name, 249, u->nick, "T :bytes sent %7.2f%s", bytes(cnt.bout), sbytes(cnt.bout));
		  numeric_sts(me.name, 249, u->nick, "T :bytes recv %7.2f%s", bytes(cnt.bin), sbytes(cnt.bin));
		  break;

	  case 'u':
		  numeric_sts(me.name, 242, u->nick, ":Services Uptime: %s", timediff(CURRTIME - me.start));
		  break;

	  case 'V':
	  case 'v':
		  if (!has_priv(u, PRIV_SERVER_AUSPEX))
			  break;

		  /* we received this command from the uplink, so,
		   * hmm, it is not idle */
		  numeric_sts(me.name, 249, u->nick, "V :%s (AutoConn.!*@*) Idle: 0 SendQ: ? Connected: %s",
				  curr_uplink->name,
				  timediff(CURRTIME - curr_uplink->conn->first_recv));
		  break;

	  default:
		  break;
	}

	numeric_sts(me.name, 219, u->nick, "%c :End of /STATS report", req);
}

void handle_whois(user_t *u, char *target)
{
	user_t *t = user_find_named(target);

	if (u == NULL)
		return;
	if (floodcheck(u, NULL))
		return;

	if (t != NULL)
	{
		numeric_sts(me.name, 311, u->nick, "%s %s %s * :%s", t->nick, t->user, t->vhost, t->gecos);
		/* channels purposely omitted */
		numeric_sts(me.name, 312, u->nick, "%s %s :%s", t->nick, t->server->name, t->server->desc);
		if (is_ircop(t))
			numeric_sts(me.name, 313, u->nick, "%s :is an IRC Operator", t->nick);
		if (t->myuser)
			numeric_sts(me.name, 330, u->nick, "%s %s :is logged in as", t->nick, t->myuser->name);
	}
	else
		numeric_sts(me.name, 401, u->nick, "%s :No such nick", target);
	numeric_sts(me.name, 318, u->nick, "%s :End of WHOIS", target);
}

static void single_trace(user_t *u, user_t *t)
{
	if (is_ircop(t))
		numeric_sts(me.name, 204, u->nick, "Oper service %s[%s@%s] (255.255.255.255) 0 0", t->nick, t->user, t->vhost);
	else
		numeric_sts(me.name, 205, u->nick, "User service %s[%s@%s] (255.255.255.255) 0 0", t->nick, t->user, t->vhost);
}

/* target -> object to trace
 * dest -> server to execute command on
 */
void handle_trace(user_t *u, char *target, char *dest)
{
	user_t *t;
	node_t *n;
	int nusers;

	if (u == NULL)
		return;
	if (floodcheck(u, NULL))
		return;
	if (!match(target, me.name) || !irccasecmp(target, ME))
	{
		nusers = cnt.user;
		LIST_FOREACH(n, me.me->userlist.head)
		{
			t = n->data;
			single_trace(u, t);
			nusers--;
		}
		if (has_priv(u, PRIV_SERVER_AUSPEX))
			numeric_sts(me.name, 206, u->nick, "Serv uplink %dS %dC %s *!*@%s 0", cnt.server - 1, nusers, me.actual, me.name);
		target = me.name;
	}
	else
	{
		t = dest != NULL ? user_find_named(target) : user_find(target);
		if (t != NULL && t->server == me.me)
		{
			single_trace(u, t);
			target = t->nick;
		}
	}
	numeric_sts(me.name, 262, u->nick, "%s :End of TRACE", target);
}

void handle_motd(user_t *u)
{
	FILE *f;
	char lbuf[BUFSIZE];
	char ebuf[BUFSIZE];
	char nbuf[BUFSIZE];
	char cbuf[BUFSIZE];

	if (u == NULL)
		return;
	if (floodcheck(u, NULL))
		return;

	f = fopen("etc/atheme.motd", "r");
	if (!f)
	{
		numeric_sts(me.name, 422, u->nick, ":The MOTD file is unavailable.");
		return;
	}

	snprintf(ebuf, BUFSIZE, "%d", config_options.expire / 86400);
	snprintf(nbuf, BUFSIZE, "%d", cnt.myuser);
	snprintf(cbuf, BUFSIZE, "%d", cnt.mychan);

	numeric_sts(me.name, 375, u->nick, ":- %s Message of the Day -", me.name);

	while (fgets(lbuf, BUFSIZE, f))
	{
		strip(lbuf);

		replace(lbuf, BUFSIZE, "&network&", me.netname);
		replace(lbuf, BUFSIZE, "&expiry&", ebuf);
		replace(lbuf, BUFSIZE, "&myusers&", nbuf);
		replace(lbuf, BUFSIZE, "&mychans&", cbuf);

		numeric_sts(me.name, 372, u->nick, ":- %s", lbuf);
	}

	numeric_sts(me.name, 376, u->nick, ":End of the message of the day.");

	fclose(f);
}

void handle_message(char *origin, char *target, boolean_t is_notice, char *message)
{
	user_t *u;
	user_t *t;
	service_t *sptr;
	char *vec[3];

	/* no prefix, so from server */
	if (!origin)
		return;

	if (!(u = user_find(origin)))
	{
		/* don't complain about notices from servers */
		if (!is_notice || !server_find(origin))
			slog(LG_DEBUG, "handle_privmsg(): got message from nonexistant user `%s'", origin);
		return;
	}

	/* If target is a channel and fantasy commands are enabled,
	 * this will return chanserv
	 */
	sptr = find_service(target);
	if (sptr == NULL)
	{
		if (*target != '#')
			slog(LG_DEBUG, "handle_privmsg(): got message to nonexistant user `%s'", target);
		return;
	}

	if (sptr)
	{
		t = sptr->me;

		/* Run it through flood checks. Channel commands are checked
		 * separately.
		 */
		if (t != NULL && *target != '#' && floodcheck(u, t))
			return;

		if (!is_notice && config_options.secure && *target != '#' && irccasecmp(target, sptr->disp))
		{
			notice(t->nick, u->nick, "For security reasons, \2/msg %s\2 has been disabled."
					" Use \2/%s%s <command>\2 to send a command.",
					sptr->me->nick, (ircd->uses_rcommand ? "" : "msg "), sptr->disp);
			return;
		}

		vec[0] = target;
		vec[1] = message;
		vec[2] = NULL;
		if (is_notice)
			sptr->notice_handler(u->nick, 2, vec);
		else
			sptr->handler(u->nick, 2, vec);
	}
}

void handle_topic(channel_t *c, char *setter, time_t ts, char *topic)
{
	char newsetter[HOSTLEN], *p;

	/* setter can be a nick, nick!user@host or server.
	 * strip off !user@host part if it's there
	 * (do we really want this?) */
	strlcpy(newsetter, setter, sizeof newsetter);
	p = strchr(newsetter, '!');
	if (p != NULL)
		*p = '\0';
	/* drop identical topics from servers or chanserv, and ones with
	 * identical topicts */
	if (topic != NULL && c->topic != NULL && !strcmp(topic, c->topic)
			&& (strchr(newsetter, '.') ||
				(chansvs.nick && !irccasecmp(newsetter, chansvs.nick)) ||
				ts == c->topicts))
		return;
	if (c->topic != NULL)
		free(c->topic);
	if (c->topic_setter != NULL)
		free(c->topic_setter);
	if (topic != NULL && topic[0] != '\0')
	{
		c->topic = sstrdup(topic);
		c->topic_setter = sstrdup(newsetter);
	}
	else
		c->topic = c->topic_setter = NULL;
	c->topicts = ts;

	hook_call_event("channel_topic", c);
}

void handle_kill(char *origin, char *victim, char *reason)
{
	char *source;
	server_t *source_server;
	user_t *source_user;
	user_t *u;

	if (origin == NULL)
		source = me.actual;
	else if ((source_server = server_find(origin)) != NULL)
		source = source_server->name;
	else if ((source_user = user_find(origin)) != NULL)
		source = source_user->nick;
	else
		source = origin;

	u = user_find(victim);
	if (u == NULL)
		slog(LG_DEBUG, "handle_kill(): %s killed unknown user %s (%s)", origin, victim, reason);
	else if (u->server == me.me)
	{
		slog(LG_INFO, "handle_kill(): %s killed service %s (%s)", source, u->nick, reason);
		reintroduce_user(u);
	}
	else
	{
		slog(LG_DEBUG, "handle_kill(): %s killed user %s (%s)", source, u->nick, reason);
		user_delete(u);
	}
}

void handle_eob(server_t *s)
{
	node_t *n;
	server_t *s2;

	if (s == NULL)
		return;
	if (s->flags & SF_EOB)
		return;
	slog(LG_DEBUG, "handle_eob(): end of burst from %s", s->name);
	hook_call_event("server_eob", s);
	s->flags |= SF_EOB;
	/* convert P10 style EOB to ircnet/ratbox style */
	LIST_FOREACH(n, s->children.head)
	{
		s2 = n->data;
		if (s2->flags & SF_EOB2)
			handle_eob(s2);
	}
}

/* Received a message from a user, check if they are flooding
 * Returns true if the message should be ignored.
 * u - user sending the message
 * t - target of the message (to be used in warning the user, may be NULL
 *     to use the server name)
 */
int floodcheck(user_t *u, user_t *t)
{
	char *from;

	if (t == NULL)
		from = me.name;
	else if (t->server == me.me)
		from = t->nick;
	else
	{
		slog(LG_ERROR, "BUG: tried to floodcheck message to non-service %s", t->nick);
		return 0;
	}

	/* Check if we match a services ignore */
	if (svsignore_find(u))
		return 1;

	if (config_options.flood_msgs)
	{
		/* check if they're being ignored */
		if (u->offenses > 10)
		{
			if ((CURRTIME - u->lastmsg) > 30)
			{
				u->offenses -= 10;
				u->lastmsg = CURRTIME;
				u->msgs = 0;
			}
			else
				return 1;
		}

		if ((CURRTIME - u->lastmsg) > config_options.flood_time)
		{
			u->lastmsg = CURRTIME;
			u->msgs = 0;
		}

		u->msgs++;

		if (u->msgs > config_options.flood_msgs)
		{
			/* they're flooding. */
			/* perhaps allowed to? -- jilles */
			if (has_priv(u, PRIV_FLOOD))
			{
				u->msgs = 0;
				return 0;
			}
			if (!u->offenses)
			{
				/* ignore them the first time */
				u->lastmsg = CURRTIME;
				u->msgs = 0;
				u->offenses = 11;

				/* ok to use nick here, notice() will
				 * change it to UID if necessary -- jilles */
				notice(from, u->nick, "You have triggered services flood protection.");
				notice(from, u->nick, "This is your first offense. You will be ignored for 30 seconds.");

				snoop("FLOOD: \2%s\2", u->nick);

				return 1;
			}

			if (u->offenses == 1)
			{
				/* ignore them the second time */
				u->lastmsg = CURRTIME;
				u->msgs = 0;
				u->offenses = 12;

				notice(from, u->nick, "You have triggered services flood protection.");
				notice(from, u->nick, "This is your last warning. You will be ignored for 30 seconds.");

				snoop("FLOOD: \2%s\2", u->nick);

				return 1;
			}

			if (u->offenses == 2)
			{
				kline_t *k;

				/* kline them the third time */
				k = kline_add("*", u->host, "ten minute ban: flooding services", 600);
				k->setby = sstrdup(chansvs.nick);

				snoop("FLOOD:KLINE: \2%s\2", u->nick);

				return 1;
			}
		}
	}

	return 0;
}
