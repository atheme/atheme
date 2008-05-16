/*
 * atheme-services: A collection of minimalist IRC services
 * ptasks.c: Implementation of common protocol tasks.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "privs.h"

void handle_info(user_t *u)
{
	unsigned int i;

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

	numeric_sts(me.name, 351, u->nick, "atheme-%s. %s :%s%s%s%s%s%s%s%s%s%s [%s]",
		    version, me.name, 
		    (match_mapping) ? "A" : "",
		    log_debug_enabled() ? "d" : "",
		    (me.auth) ? "e" : "",
		    (config_options.flood_msgs) ? "F" : "",
		    (config_options.leave_chans) ? "l" : "", 
		    (config_options.join_chans) ? "j" : "", 
		    (chansvs.changets) ? "t" : "",
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

static void dictionary_stats_cb(const char *line, void *privdata)
{
	numeric_sts(me.name, 249, ((user_t *)privdata)->nick, "B :%s", line);
}

static void connection_stats_cb(const char *line, void *privdata)
{
	numeric_sts(me.name, 249, ((user_t *)privdata)->nick, "F :%s", line);
}

void handle_stats(user_t *u, char req)
{
	kline_t *k;
	node_t *n;
	uplink_t *uplink;
	soper_t *soper;
	int i, j;
	char fl[10];

	if (floodcheck(u, NULL))
		return;
	logcommand_user(NULL, u, CMDLOG_GET, "STATS %c", req);

	switch (req)
	{
	  case 'B':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
			  break;

		  mowgli_patricia_stats(userlist, dictionary_stats_cb, u);
		  mowgli_patricia_stats(chanlist, dictionary_stats_cb, u);
		  mowgli_patricia_stats(servlist, dictionary_stats_cb, u);
		  mowgli_patricia_stats(mulist, dictionary_stats_cb, u);
		  mowgli_patricia_stats(nicklist, dictionary_stats_cb, u);
		  mowgli_patricia_stats(mclist, dictionary_stats_cb, u);
		  break;

	  case 'C':
	  case 'c':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
			  break;

		  LIST_FOREACH(n, uplinks.head)
		  {
			  uplink = (uplink_t *)n->data;
			  numeric_sts(me.name, 213, u->nick, "C *@127.0.0.1 A %s %d uplink", uplink->name, uplink->port);
		  }
		  break;

	  case 'E':
	  case 'e':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
			  break;

		  numeric_sts(me.name, 249, u->nick, "E :Last event to run: %s", last_event_ran);

		  numeric_sts(me.name, 249, u->nick, "E :%-28s %s", "Operation", "Next Execution");
		  for (i = 0; i < MAX_EVENTS; i++)
		  {
			  if (event_table[i].active)
				  numeric_sts(me.name, 249, u->nick, "E :%-28s %4ld seconds (%ld)", event_table[i].name, (long)(event_table[i].when - CURRTIME), (long)event_table[i].frequency);
		  }

		  break;

	  case 'f':
	  case 'F':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
			  break;

		  connection_stats(connection_stats_cb, u);
		  break;

	  case 'H':
	  case 'h':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
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
		  if (!has_priv_user(u, PRIV_AKILL))
			  break;

		  LIST_FOREACH(n, klnlist.head)
		  {
			  k = (kline_t *)n->data;

			  numeric_sts(me.name, 216, u->nick, "%c %s * %s :%s",
					  k->duration ? 'k' : 'K',
					  k->host, k->user, k->reason);
		  }

		  break;

	  case 'o':
	  case 'O':
		  if (!has_priv_user(u, PRIV_VIEWPRIVS))
			  break;

		  LIST_FOREACH(n, soperlist.head)
		  {
			  soper = n->data;

			  j = 0;
			  if (!(soper->flags & SOPER_CONF))
				  fl[j++] = 'D';
			  if (soper->operclass != NULL && soper->operclass->flags & OPERCLASS_NEEDOPER)
				  fl[j++] = 'O';
			  if (j == 0)
				  fl[j++] = '*';
			  fl[j] = '\0';
			  numeric_sts(me.name, 243, u->nick, "O *@* %s %s %s %s",
					  fl, soper->myuser ? soper->myuser->name : soper->name,
					  soper->operclass ? soper->operclass->name : soper->classname, "-1");
		  }
		  break;

	  case 'T':
	  case 't':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
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
		  numeric_sts(me.name, 249, u->nick, "T :chanuser   %7d", cnt.chanuser);
		  numeric_sts(me.name, 249, u->nick, "T :myuser     %7d", cnt.myuser);
		  numeric_sts(me.name, 249, u->nick, "T :myuser_acc %7d", cnt.myuser_access);
		  numeric_sts(me.name, 249, u->nick, "T :mynick     %7d", cnt.mynick);
		  numeric_sts(me.name, 249, u->nick, "T :myuser_nam %7d", cnt.myuser_name);
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
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
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

void handle_whois(user_t *u, const char *target)
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
		if (t->flags & UF_AWAY)
			numeric_sts(me.name, 301, u->nick, "%s :Gone", t->nick);
		if (is_ircop(t))
			numeric_sts(me.name, 313, u->nick, "%s :%s", t->nick, is_internal_client(t) ? "is a Network Service" : "is an IRC Operator");
		if (t->myuser)
			numeric_sts(me.name, 330, u->nick, "%s %s :is logged in as", t->nick, t->myuser->name);
	}
	else
		numeric_sts(me.name, 401, u->nick, "%s :No such nick", target);
	numeric_sts(me.name, 318, u->nick, "%s :End of WHOIS", target);
}

static void single_trace(user_t *u, user_t *t)
{
	const char *classname;

	classname = t->flags & UF_ENFORCER ? "enforcer" : "service";
	if (is_ircop(t))
		numeric_sts(me.name, 204, u->nick, "Oper %s %s[%s@%s] (255.255.255.255) 0 0", classname, t->nick, t->user, t->vhost);
	else
		numeric_sts(me.name, 205, u->nick, "User %s %s[%s@%s] (255.255.255.255) 0 0", classname, t->nick, t->user, t->vhost);
}

/* target -> object to trace
 * dest -> server to execute command on
 */
void handle_trace(user_t *u, const char *target, const char *dest)
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
			if (is_ircop(t))
				single_trace(u, t);
			nusers--;
		}
		if (has_priv_user(u, PRIV_SERVER_AUSPEX))
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
	char nebuf[BUFSIZE];
	char cebuf[BUFSIZE];
	char ubuf[BUFSIZE];
	char cbuf[BUFSIZE];
	char nbuf[BUFSIZE];

	if (u == NULL)
		return;
	if (floodcheck(u, NULL))
		return;

	f = fopen(SYSCONFDIR "/atheme.motd", "r");
	if (!f)
	{
		numeric_sts(me.name, 422, u->nick, ":The MOTD file is unavailable.");
		return;
	}

	snprintf(nebuf, BUFSIZE, "%d", nicksvs.expiry / 86400);
	snprintf(cebuf, BUFSIZE, "%d", chansvs.expiry / 86400);
	snprintf(ubuf, BUFSIZE, "%d", cnt.myuser);
	snprintf(nbuf, BUFSIZE, "%d", nicksvs.no_nick_ownership ? 0 : cnt.mynick);
	snprintf(cbuf, BUFSIZE, "%d", cnt.mychan);

	numeric_sts(me.name, 375, u->nick, ":- %s Message of the Day -", me.name);

	while (fgets(lbuf, BUFSIZE, f))
	{
		strip(lbuf);

		replace(lbuf, BUFSIZE, "&network&", me.netname);
		replace(lbuf, BUFSIZE, "&nickexpiry&", nebuf);
		replace(lbuf, BUFSIZE, "&chanexpiry&", cebuf);
		replace(lbuf, BUFSIZE, "&myusers&", ubuf);
		replace(lbuf, BUFSIZE, "&mynicks&", nbuf);
		replace(lbuf, BUFSIZE, "&mychans&", cbuf);

		numeric_sts(me.name, 372, u->nick, ":- %s", lbuf);
	}

	numeric_sts(me.name, 376, u->nick, ":End of the message of the day.");

	fclose(f);
}

void handle_away(user_t *u, const char *message)
{
	if (message == NULL || *message == '\0')
	{
		if (u->flags & UF_AWAY)
		{
			u->flags &= ~UF_AWAY;
			hook_call_event("user_away", u);
		}
	}
	else
	{
		if (!(u->flags & UF_AWAY))
		{
			u->flags |= UF_AWAY;
			hook_call_event("user_away", u);
		}
	}
}

static void
handle_channel_message(sourceinfo_t *si, char *target, boolean_t is_notice, char *message)
{
	char *vec[3];
	hook_cmessage_data_t cdata;
	node_t *n;

	/* Call hook here */
	cdata.u = si->su;
	cdata.c = channel_find(target);
	cdata.msg = message;

	/* No such channel, ignore... */
	if (cdata.c == NULL)
		return;

	hook_call_event("channel_message", &cdata);

	vec[0] = target;
	vec[1] = message;
	vec[2] = NULL;

	LIST_FOREACH(n, cdata.c->members.head)
	{
		chanuser_t *cu = (chanuser_t *) n->data;

		if (!is_internal_client(cu->user))
			continue;

		si->service = find_service(cu->user->nick);

		if (si->service == NULL)
			continue;

		if (si->service->chanmsg == FALSE)
			continue;

		if (is_notice)
			si->service->notice_handler(si, 2, vec);
		else
			si->service->handler(si, 2, vec);
	}
}

void handle_message(sourceinfo_t *si, char *target, boolean_t is_notice, char *message)
{
	char *vec[3];

	/* message from server, ignore */
	if (si->su == NULL)
		return;

	/* if this is a channel, handle it differently. */
	if (*target == '#')
	{
		handle_channel_message(si, target, is_notice, message);
		return;
	}

	si->service = find_service(target);

	if (si->service == NULL)
	{
		if (!is_notice && (isalnum(target[0]) || strchr("[\\]^_`{|}~", target[0])))
		{
			/* If it's not a notice and looks like a nick or
			 * user@server, send back an error message */
			if (strchr(target, '@') || !ircd->uses_uid || (!ircd->uses_p10 && !isdigit(target[0])))
				numeric_sts(me.name, 401, si->su->nick, "%s :No such nick", target);
			else
				numeric_sts(me.name, 401, si->su->nick, "* :Target left IRC. Failed to deliver: [%.20s]", message);
		}
		return;
	}

	/* Run it through flood checks. Channel commands are checked
	 * separately.
	 */
	if (si->service->me != NULL && floodcheck(si->su, si->service->me))
		return;

	if (!is_notice && config_options.secure && irccasecmp(target, si->service->disp))
	{
		notice(si->service->me->nick, si->su->nick, "For security reasons, \2/msg %s\2 has been disabled."
				" Use \2/%s%s <command>\2 to send a command.",
				si->service->me->nick, (ircd->uses_rcommand ? "" : "msg "), si->service->disp);
		return;
	}

	vec[0] = target;
	vec[1] = message;
	vec[2] = NULL;
	if (is_notice)
		si->service->notice_handler(si, 2, vec);
	else
		si->service->handler(si, 2, vec);
}

void handle_topic_from(sourceinfo_t *si, channel_t *c, char *setter, time_t ts, char *topic)
{
	hook_channel_topic_check_t hdata;

	if (topic != NULL && topic[0] == '\0')
		topic = NULL;
	hdata.u = si->su;
	hdata.s = si->s;
	hdata.c = c;
	hdata.setter = setter;
	hdata.ts = ts;
	hdata.topic = topic;
	hdata.approved = 0;
	if (topic != NULL ? c->topic == NULL || strcmp(topic, c->topic) : c->topic != NULL)
	{
		/* Only call the hook if the topic actually changed */
		hook_call_event("channel_can_change_topic", &hdata);
	}
	if (hdata.approved == 0)
	{
		if (topic == hdata.topic || !strcmp(topic, hdata.topic))
			/* Allowed, process the change further */
			handle_topic(c, setter, ts, topic);
		else
		{
			/* Allowed, but topic tweaked */
			handle_topic(c, setter, ts, hdata.topic);
			topic_sts(c, setter, ts, ts, hdata.topic);
		}
	}
	else
	{
		/* Not allowed, change it back */
		if (c->topic != NULL)
			topic_sts(c, c->topic_setter, c->topicts, ts, c->topic);
		else
		{
			/* Ick, there was no topic */
			topic_sts(c, chansvs.nick != NULL ? chansvs.nick : me.name, ts - 1, ts, "");
		}
	}
}

void handle_topic(channel_t *c, const char *setter, time_t ts, const char *topic)
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

static const char *skip_kill_path(const char *reason)
{
	const char *p;
	bool have_dot = FALSE;

	p = reason;
	while (*p != ' ')
	{
		if (strchr("<>()#&?%$,;", *p) || *p == '\0')
			return reason;
		if (*p == '!' || *p == '.')
			have_dot = TRUE;
		p++;
	}
	return have_dot ? p + 1 : reason;
}

void handle_kill(sourceinfo_t *si, const char *victim, const char *reason)
{
	const char *source;
	user_t *u;
	static time_t lastkill = 0;
	static unsigned int killcount = 0;

	source = get_oper_name(si);
	reason = skip_kill_path(reason);

	u = user_find(victim);
	if (u == NULL)
		slog(LG_DEBUG, "handle_kill(): %s killed unknown user %s (%s)", source, victim, reason);
	else if (u->flags & UF_ENFORCER)
	{
		slog(LG_INFO, "handle_kill(): %s killed enforcer %s (%s)", source, u->nick, reason);
		user_delete(u);
	}
	else if (u->server == me.me)
	{
		slog(LG_INFO, "handle_kill(): %s killed service %s (%s)", source, u->nick, reason);
		if (lastkill != CURRTIME && killcount < 5 + me.me->users)
			killcount = 0, lastkill = CURRTIME;
		killcount++;
		if (killcount < 5 + me.me->users)
			reintroduce_user(u);
		else
		{
			slog(LG_ERROR, "handle_kill(): services kill fight (%s -> %s), shutting down", source, u->nick);
			wallops(_("Services kill fight (%s -> %s), shutting down!"), source, u->nick);
			snoop(_("ERROR: Services kill fight (%s -> %s), shutting down!"), source, u->nick);
			runflags |= RF_SHUTDOWN;
		}
	}
	else
	{
		slog(LG_DEBUG, "handle_kill(): %s killed user %s (%s)", source, u->nick, reason);
		user_delete(u);
	}
}

server_t *handle_server(sourceinfo_t *si, const char *name, const char *sid,
		int hops, const char *desc)
{
	server_t *s = NULL;

	if (si->s != NULL)
	{
		/* A server introducing another server */
		s = server_add(name, hops, si->s->name, sid, desc);
	}
	else if (cnt.server == 1)
	{
		/* Our uplink introducing itself */
		if (irccasecmp(name, curr_uplink->name))
			slog(LG_ERROR, "handle_server(): uplink %s actually has name %s, continuing anyway", curr_uplink->name, name);
		s = server_add(name, hops, me.name, sid, desc);
		me.actual = s->name;
		me.recvsvr = TRUE;
	}
	else
		slog(LG_ERROR, "handle_server(): unregistered/unknown server attempting to introduce another server %s", name);
	return s;
}

void handle_eob(server_t *s)
{
	node_t *n;
	server_t *s2;

	if (s == NULL)
		return;
	if (s->flags & SF_EOB)
		return;
	slog(LG_NETWORK, "handle_eob(): end of burst from %s (%d users)",
			s->name, s->users);
	hook_call_event("server_eob", s);
	s->flags |= SF_EOB;
	/* convert P10 style EOB to ircnet/ratbox style */
	LIST_FOREACH(n, s->children.head)
	{
		s2 = n->data;
		if (s2->flags & SF_EOB2)
			handle_eob(s2);
	}
	/* Reseed RNG now we have a little more data to seed with */
	if (s->uplink == me.me)
		srand(rand() ^ ((CURRTIME << 20) + cnt.user + (cnt.chanuser << 12)) ^ (cnt.chan << 17) ^ ~cnt.bin);
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
	static time_t last_ignore_notice = 0;

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
	{
		if (u->msgs == 0 && last_ignore_notice != CURRTIME)
		{
			/* tell them once per session, don't flood */
			u->msgs = 1;
			last_ignore_notice = CURRTIME;
			notice(from, u->nick, _("You are on services ignore. You may not use any service."));
		}
		return 1;
	}

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

		if ((unsigned int)(CURRTIME - u->lastmsg) > config_options.flood_time)
		{
			u->lastmsg = CURRTIME;
			u->msgs = 0;
		}

		u->msgs++;

		if (u->msgs > config_options.flood_msgs)
		{
			/* they're flooding. */
			/* perhaps allowed to? -- jilles */
			if (has_priv_user(u, PRIV_FLOOD))
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
				notice(from, u->nick, _("You have triggered services flood protection."));
				notice(from, u->nick, _("This is your first offense. You will be ignored for 30 seconds."));

				snoop("FLOOD: \2%s\2", u->nick);

				return 1;
			}

			if (u->offenses == 1)
			{
				/* ignore them the second time */
				u->lastmsg = CURRTIME;
				u->msgs = 0;
				u->offenses = 12;

				notice(from, u->nick, _("You have triggered services flood protection."));
				notice(from, u->nick, _("This is your last warning. You will be ignored for 30 seconds."));

				snoop("FLOOD: \2%s\2", u->nick);

				return 1;
			}

			if (u->offenses == 2)
			{
				kline_t *k;

				/* kline them the third time */
				k = kline_add("*", u->host, "ten minute ban - flooding services", 600);
				k->setby = sstrdup(chansvs.nick);

				snoop("FLOOD:KLINE: \2%s\2", u->nick);

				return 1;
			}
		}
	}

	return 0;
}

boolean_t should_reg_umode(user_t *u)
{
	mynick_t *mn;

	if (nicksvs.me == NULL || nicksvs.no_nick_ownership ||
			u->myuser == NULL || u->myuser->flags & MU_WAITAUTH)
		return FALSE;
	mn = mynick_find(u->nick);
	return mn != NULL && mn->owner == u->myuser;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
