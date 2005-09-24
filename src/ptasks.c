/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol tasks, such as handle_stats().
 *
 * $Id: ptasks.c 2337 2005-09-24 02:01:26Z jilles $
 */

#include "atheme.h"

void handle_info(char *origin)
{
        uint8_t i;

        for (i = 0; infotext[i]; i++)
                sts(":%s 371 %s :%s", me.name, origin, infotext[i]);

        sts(":%s 374 %s :End of /INFO list", me.name, origin);
}

void handle_version(char *origin)
{
        sts(":%s 351 %s :atheme-%s. %s %s%s%s%s%s%s%s%s%s",
            me.name, origin, version, me.name,
            (match_mapping) ? "A" : "",
            (me.loglevel & LG_DEBUG) ? "d" : "",
            (me.auth) ? "e" : "",
            (config_options.flood_msgs) ? "F" : "",
            (config_options.leave_chans) ? "l" : "",
	    (config_options.join_chans) ? "j" : "",
            (!match_mapping) ? "R" : "",
            (config_options.raw) ? "r" : "",
            (runflags & RF_LIVE) ? "n" : "");
        sts(":%s 351 %s :Compile time: %s, build-id %s, build %s", me.name, origin, creation, revision, generation);
}

void handle_admin(char *origin)
{
        sts(":%s 256 %s :Administrative info about %s", me.name, origin, me.name);
        sts(":%s 257 %s :%s", me.name, origin, me.adminname);
        sts(":%s 258 %s :Atheme IRC Services (atheme-%s)", me.name, origin, version);
        sts(":%s 259 %s :<%s>", me.name, origin, me.adminemail);
}

void handle_stats(char *origin, char req)
{
	user_t *u = user_find(origin);
	kline_t *k;
	node_t *n;
	int i;

	snoop("STATS:%c: \2%s\2", req, origin);

	switch (req)
	{
	  case 'C':
	  case 'c':
		  numeric_sts(me.name, 213, u->nick, "C *@127.0.0.1 A %s %d uplink", (is_ircop(u)) ? curr_uplink->name : "127.0.0.1", curr_uplink->port);
		  break;

	  case 'E':
	  case 'e':
		  if (!is_ircop(u))
			  break;

		  numeric_sts(me.name, 249, u->nick, "E :Last event to run: %s", last_event_ran);

		  for (i = 0; i < MAX_EVENTS; i++)
		  {
			  if (event_table[i].active)
				  numeric_sts(me.name, 249, u->nick, "E :%s (%d)", event_table[i].name, event_table[i].frequency);
		  }

		  break;

	  case 'H':
	  case 'h':
		  numeric_sts(me.name, 244, u->nick, "H * * %s", (is_ircop(u)) ? curr_uplink->name : "127.0.0.1");
		  break;

	  case 'I':
	  case 'i':
		  numeric_sts(me.name, 215, u->nick, "I * * *@%s 0 nonopered", me.name);
		  break;

	  case 'K':
	  case 'k':
		  if (!is_ircop(u))
			  break;

		  LIST_FOREACH(n, klnlist.head)
		  {
			  k = (kline_t *)n->data;

			  numeric_sts(me.name, 216, u->nick, "K %s * %s :%s", k->host, k->user, k->reason);
		  }

		  break;

	  case 'T':
	  case 't':
		  if (!is_ircop(u))
			  break;

		  numeric_sts(me.name, 249, u->nick, ":event      %7d", cnt.event);
		  numeric_sts(me.name, 249, u->nick, ":sra        %7d", cnt.sra);
		  numeric_sts(me.name, 249, u->nick, ":tld        %7d", cnt.tld);
		  numeric_sts(me.name, 249, u->nick, ":kline      %7d", cnt.kline);
		  numeric_sts(me.name, 249, u->nick, ":server     %7d", cnt.server);
		  numeric_sts(me.name, 249, u->nick, ":user       %7d", cnt.user);
		  numeric_sts(me.name, 249, u->nick, ":chan       %7d", cnt.chan);
		  numeric_sts(me.name, 249, u->nick, ":myuser     %7d", cnt.myuser);
		  numeric_sts(me.name, 249, u->nick, ":mychan     %7d", cnt.mychan);
		  numeric_sts(me.name, 249, u->nick, ":chanacs    %7d", cnt.chanacs); 
		  numeric_sts(me.name, 249, u->nick, ":node       %7d", cnt.node);

		  numeric_sts(me.name, 249, u->nick, ":bytes sent %7.2f%s", bytes(cnt.bout), sbytes(cnt.bout));
		  numeric_sts(me.name, 249, u->nick, ":bytes recv %7.2f%s", bytes(cnt.bin), sbytes(cnt.bin));
		  break;

	  case 'u':
		  numeric_sts(me.name, 242, u->nick, ":Services Uptime: %s", timediff(CURRTIME - me.start));
		  break;

	  default:
		  break;
	}

	numeric_sts(me.name, 219, u->nick, "%c :End of /STATS report", req);
}

void handle_whois(char *origin, char *target)
{
	user_t *u = user_find(origin);
	user_t *t = user_find_named(target);

	if (u == NULL)
		return;
	if (t != NULL)
	{
		numeric_sts(me.name, 311, CLIENT_NAME(u), "%s %s %s * :%s",
				t->nick, t->user, t->vhost, t->gecos);
		/* channels purposely omitted */
		numeric_sts(me.name, 312, CLIENT_NAME(u), "%s %s :%s", t->nick, t->server->name,
				t->server->desc);
		if (is_ircop(t))
			numeric_sts(me.name, 313, CLIENT_NAME(u), "%s :is an IRC Operator",
					t->nick);
		if (t->myuser)
			numeric_sts(me.name, 330, CLIENT_NAME(u), "%s %s :is logged in as",
					t->nick, t->myuser->name);
	}
	else
		numeric_sts(me.name, 401, CLIENT_NAME(u), "%s :No such nick", target);
	numeric_sts(me.name, 318, CLIENT_NAME(u), "%s :End of WHOIS", target);
}

static void single_trace(user_t *u, user_t *t)
{
	if (is_ircop(t))
		numeric_sts(me.name, 204, CLIENT_NAME(u), "Oper service %s[%s@%s] (255.255.255.255) 0 0",
				t->nick, t->user, t->vhost);
	else
		numeric_sts(me.name, 205, CLIENT_NAME(u), "User service %s[%s@%s] (255.255.255.255) 0 0",
				t->nick, t->user, t->vhost);
}

/* target -> object to trace
 * dest -> server to execute command on
 */
void handle_trace(char *origin, char *target, char *dest)
{
	user_t *u = user_find(origin);
	user_t *t;
	node_t *n;
	int nusers;

	if (u == NULL)
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
		if (is_ircop(u))
			numeric_sts(me.name, 206, CLIENT_NAME(u), "Serv uplink %dS %dC %s *!*@%s 0", cnt.server - 1, nusers, me.actual, me.name);
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
	numeric_sts(me.name, 262, CLIENT_NAME(u), "%s :End of TRACE", target);
}
