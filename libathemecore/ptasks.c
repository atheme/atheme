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
 * ptasks.c: Implementation of common protocol tasks.
 */

#include <atheme.h>
#include "internal.h"

void
handle_info(struct user *u)
{
	unsigned int i;

	if (u == NULL)
		return;
	if (floodcheck(u, NULL))
		return;

	for (i = 0; infotext[i]; i++)
		numeric_sts(me.me, 371, u, ":%s", infotext[i]);

	numeric_sts(me.me, 374, u, ":End of /INFO list");
}

const char *
get_build_date(void)
{
	const char *build_date = "<unknown>";

#ifdef ATHEME_ENABLE_REPRODUCIBLE_BUILDS
#  ifdef SOURCE_DATE_EPOCH
	const time_t build_time = SOURCE_DATE_EPOCH;
	const struct tm *const tm = localtime(&build_time);
	static char datebuf[BUFSIZE];

	if (tm && strftime(datebuf, sizeof datebuf, "%b %e %Y", tm) != 0)
		build_date = datebuf;
#  endif /* SOURCE_DATE_EPOCH */
#else /* ATHEME_ENABLE_REPRODUCIBLE_BUILDS */
#  ifdef __DATE__
	build_date = __DATE__;
#  endif
#endif /* !ATHEME_ENABLE_REPRODUCIBLE_BUILDS */

	return build_date;
}

int
get_version_string(char *buf, size_t bufsize)
{
	const struct crypt_impl *const ci = crypt_get_default_provider();
	const char *const ci_id = ci ? ci->id : "<none>";

	return snprintf(buf, bufsize, "%s-%s %s :%s %s [%s] [enc:%s] Build Date: %s",
	                              PACKAGE_TARNAME, PACKAGE_VERSION, me.name, revision,
	                              get_conf_opts(), ircd->ircdname, ci_id, get_build_date());
}

void
handle_version(struct user *u)
{
	if (u == NULL)
		return;
	if (floodcheck(u, NULL))
		return;

	char ver[BUFSIZE];
	get_version_string(ver, sizeof(ver));
	numeric_sts(me.me, 351, u, "%s", ver);
}

void
handle_admin(struct user *u)
{

	if (u == NULL)
		return;

	if (floodcheck(u, NULL))
		return;

	numeric_sts(me.me, 256, u, ":Administrative info about %s", me.name);
	numeric_sts(me.me, 257, u, ":%s", me.adminname);
	numeric_sts(me.me, 258, u, ":%s (%s)", PACKAGE_NAME, PACKAGE_VERSION);
	numeric_sts(me.me, 259, u, ":<%s>", me.adminemail);
}

static void
dictionary_stats_cb(const char *line, void *privdata)
{
	numeric_sts(me.me, 249, ((struct user *)privdata), "B :%s", line);
}

static void
connection_stats_cb(const char *line, void *privdata)
{
	numeric_sts(me.me, 249, ((struct user *)privdata), "F :%s", line);
}

void
handle_stats(struct user *u, char req)
{
	struct kline *k;
	struct xline *x;
	struct qline *q;
	mowgli_node_t *n;
	struct uplink *uplink;
	struct soper *soper;
	int j;
	char fl[10];

	if (floodcheck(u, NULL))
		return;

	logcommand_user(NULL, u, CMDLOG_GET, "STATS: \2%c\2", req);

	switch (req)
	{
	  case 'B':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
			  break;

		  mowgli_patricia_stats(userlist, dictionary_stats_cb, u);
		  mowgli_patricia_stats(chanlist, dictionary_stats_cb, u);
		  mowgli_patricia_stats(servlist, dictionary_stats_cb, u);
		  myentity_stats(dictionary_stats_cb, u);
		  mowgli_patricia_stats(nicklist, dictionary_stats_cb, u);
		  mowgli_patricia_stats(mclist, dictionary_stats_cb, u);
		  break;

	  case 'C':
	  case 'c':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
			  break;

		  MOWGLI_ITER_FOREACH(n, uplinks.head)
		  {
			  uplink = (struct uplink *)n->data;
			  numeric_sts(me.me, 213, u, "C *@%s A %s %u uplink", uplink->host, uplink->name, uplink->port);
		  }
		  break;

	  case 'E':
	  case 'e':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
			  break;

		  numeric_sts(me.me, 249, u, "E :Last event to run: %s", base_eventloop->last_ran);

		  numeric_sts(me.me, 249, u, "E :%-28s %s", "Operation", "Next Execution");
		  MOWGLI_ITER_FOREACH(n, base_eventloop->timer_list.head)
		  {
			  mowgli_eventloop_timer_t *timer = n->data;

			  if (timer->active)
				  numeric_sts(me.me, 249, u, "E :%-28s %4ld seconds (%ld)", timer->name, (long)(timer->deadline - mowgli_eventloop_get_time(base_eventloop)), (long)timer->frequency);
		  }

		  break;

	  case 'F':
	  case 'f':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
			  break;

		  connection_stats(connection_stats_cb, u);
		  break;

	  case 'H':
	  case 'h':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
			  break;

		  MOWGLI_ITER_FOREACH(n, uplinks.head)
		  {
			  uplink = (struct uplink *)n->data;
			  numeric_sts(me.me, 244, u, "H * * %s", uplink->name);
		  }
		  break;

	  case 'I':
	  case 'i':
		  numeric_sts(me.me, 215, u, "I * * *@%s 0 nonopered", me.name);
		  break;

#ifdef OBJECT_DEBUG
	  case 'J':
	  case 'j':
		  MOWGLI_ITER_FOREACH(n, object_list.head)
		  {
			  struct atheme_object *obj = n->data;
			  numeric_sts(me.me, 249, u, "J :object:%p refs:%d destructor:%p",
				      obj, obj->refcount, obj->destructor);
		  }
		  break;
#endif

	  case 'K':
	  case 'k':
		  if (!has_priv_user(u, PRIV_AKILL))
			  break;

		  MOWGLI_ITER_FOREACH(n, klnlist.head)
		  {
			  k = (struct kline *)n->data;

			  numeric_sts(me.me, 216, u, "%c %s * %s :%s",
					  k->duration ? 'k' : 'K',
					  k->host, k->user, k->reason);
		  }

		  break;

	  case 'O':
	  case 'o':
		  if (!has_priv_user(u, PRIV_VIEWPRIVS))
			  break;

		  MOWGLI_ITER_FOREACH(n, soperlist.head)
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
			  numeric_sts(me.me, 243, u, "O *@* %s %s %s %s",
					  fl, soper->myuser ? entity(soper->myuser)->name : soper->name,
					  soper->operclass ? soper->operclass->name : soper->classname, "-1");
		  }
		  break;

	  case 'T':
	  case 't':
		  // These 3 are not sensitive; and can already be obtained with /LUSERS on IRC
		  numeric_sts(me.me, 249, u, "T :server     %7u", cnt.server);
		  numeric_sts(me.me, 249, u, "T :user       %7u", cnt.user);
		  numeric_sts(me.me, 249, u, "T :chan       %7u", cnt.chan);

		  // These 3 are not sensitive
		  numeric_sts(me.me, 249, u, "T :myuser     %7u", cnt.myuser);
		  numeric_sts(me.me, 249, u, "T :mynick     %7u", cnt.mynick);
		  numeric_sts(me.me, 249, u, "T :mychan     %7u", cnt.mychan);

		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
			  break;

		  numeric_sts(me.me, 249, u, "T :event      %7u", claro_state.event);
		  numeric_sts(me.me, 249, u, "T :node       %7u", claro_state.node);
		  numeric_sts(me.me, 249, u, "T :connection %7zu", connection_count());
		  numeric_sts(me.me, 249, u, "T :operclass  %7u", cnt.operclass);
		  numeric_sts(me.me, 249, u, "T :soper      %7u", cnt.soper);
		  numeric_sts(me.me, 249, u, "T :tld        %7u", cnt.tld);
		  numeric_sts(me.me, 249, u, "T :kline      %7u", cnt.kline);
		  numeric_sts(me.me, 249, u, "T :xline      %7u", cnt.xline);
		  numeric_sts(me.me, 249, u, "T :chanuser   %7u", cnt.chanuser);
		  numeric_sts(me.me, 249, u, "T :myuser_acc %7u", cnt.myuser_access);
		  numeric_sts(me.me, 249, u, "T :myuser_nam %7u", cnt.myuser_name);
		  numeric_sts(me.me, 249, u, "T :chanacs    %7u", cnt.chanacs);

#ifdef OBJECT_DEBUG
		  numeric_sts(me.me, 249, u, "T :objects    %7zu", MOWGLI_LIST_LENGTH(&object_list));
#endif

		  numeric_sts(me.me, 249, u, "T :bytes sent %7.2f%s", (double) bytes(cnt.bout), sbytes(cnt.bout));
		  numeric_sts(me.me, 249, u, "T :bytes recv %7.2f%s", (double) bytes(cnt.bin), sbytes(cnt.bin));
		  break;

	  case 'u':
		  numeric_sts(me.me, 242, u, ":Services Uptime: %s", timediff(CURRTIME - me.start));
		  break;

	  case 'V':
	  case 'v':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
			  break;

		  /* we received this command from the uplink, so,
		   * hmm, it is not idle */
		  numeric_sts(me.me, 249, u, "V :%s (AutoConn.!*@*) Idle: 0 SendQ: ? Connected: %s",
				  curr_uplink->name,
				  timediff(CURRTIME - curr_uplink->conn->first_recv));
		  break;

	  case 'Q':
	  case 'q':
		  if (!has_priv_user(u, PRIV_MASS_AKILL))
			  break;

		  MOWGLI_ITER_FOREACH(n, qlnlist.head)
		  {
			  q = (struct qline *)n->data;

			  numeric_sts(me.me, 217, u, "%c %d %s :%s",
					  q->duration ? 'q' : 'Q',
					  0, /* hit count */
					  q->mask, q->reason);
		  }

		  break;

	  case 'X':
	  case 'x':
		  if (!has_priv_user(u, PRIV_MASS_AKILL))
			  break;

		  MOWGLI_ITER_FOREACH(n, xlnlist.head)
		  {
			  x = (struct xline *)n->data;

			  numeric_sts(me.me, 247, u, "%c %d %s :%s",
					  x->duration ? 'x' : 'X',
					  0, /* hit count */
					  x->realname, x->reason);
		  }

		  break;

	  case 'Y':
	  case 'y':
		  if (!has_priv_user(u, PRIV_SERVER_AUSPEX))
			  break;

		  numeric_sts(me.me, 218, u, "Y uplink 300 %u 1 %u 0.0 0.0 1",
				  me.recontime, config_options.uplink_sendq_limit);
		  break;

	  default:
		  break;
	}

	numeric_sts(me.me, 219, u, "%c :End of /STATS report", req);
}

void
handle_whois(struct user *u, const char *target)
{
	struct user *t = user_find_named(target);

	if (u == NULL)
		return;
	if (floodcheck(u, NULL))
		return;

	if (t != NULL)
	{
		numeric_sts(me.me, 311, u, "%s %s %s * :%s", t->nick, t->user, t->vhost, t->gecos);
		/* channels purposely omitted */
		numeric_sts(me.me, 312, u, "%s %s :%s", t->nick, t->server->name, t->server->desc);
		if (t->flags & UF_AWAY)
			numeric_sts(me.me, 301, u, "%s :Gone", t->nick);
		if (is_service(t))
			numeric_sts(me.me, 313, u, "%s :%s", t->nick, config_options.servicestring);
		else if (is_ircop(t) && !config_options.hide_opers)
			numeric_sts(me.me, 313, u, "%s :%s", t->nick, config_options.operstring);
		if (t->myuser && !(t->myuser->flags & MU_WAITAUTH))
			numeric_sts(me.me, 330, u, "%s %s :is logged in as", t->nick, entity(t->myuser)->name);
	}
	else
		numeric_sts(me.me, 401, u, "%s :No such nick", target);
	numeric_sts(me.me, 318, u, "%s :End of WHOIS", target);
}

static void
single_trace(struct user *u, struct user *t)
{
	const char *classname;

	classname = t->flags & UF_ENFORCER ? "enforcer" : "service";
	if (is_ircop(t))
		numeric_sts(me.me, 204, u, "Oper %s %s[%s@%s] (255.255.255.255) 0 0", classname, t->nick, t->user, t->vhost);
	else
		numeric_sts(me.me, 205, u, "User %s %s[%s@%s] (255.255.255.255) 0 0", classname, t->nick, t->user, t->vhost);
}

/* target -> object to trace
 * dest -> server to execute command on
 */
void
handle_trace(struct user *u, const char *target, const char *dest)
{
	struct user *t;
	mowgli_node_t *n;
	unsigned int nusers;

	if (u == NULL)
		return;
	if (floodcheck(u, NULL))
		return;
	if (!match(target, me.name) || !irccasecmp(target, ME))
	{
		nusers = cnt.user;
		MOWGLI_ITER_FOREACH(n, me.me->userlist.head)
		{
			t = n->data;
			if (is_ircop(t))
				single_trace(u, t);
			nusers--;
		}
		if (has_priv_user(u, PRIV_SERVER_AUSPEX))
			numeric_sts(me.me, 206, u, "Serv uplink %uS %uC %s *!*@%s 0", cnt.server - 1, nusers, me.actual, me.name);
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
	numeric_sts(me.me, 262, u, "%s :End of TRACE", target);
}

void
handle_motd(struct user *u)
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
		numeric_sts(me.me, 422, u, ":The MOTD file is unavailable.");
		return;
	}

	snprintf(nebuf, BUFSIZE, "%u", nicksvs.expiry / 86400);
	snprintf(cebuf, BUFSIZE, "%u", chansvs.expiry / 86400);
	snprintf(ubuf, BUFSIZE, "%u", cnt.myuser);
	snprintf(nbuf, BUFSIZE, "%u", nicksvs.no_nick_ownership ? 0U : cnt.mynick);
	snprintf(cbuf, BUFSIZE, "%u", cnt.mychan);

	numeric_sts(me.me, 375, u, ":- %s Message of the Day -", me.name);

	while (fgets(lbuf, BUFSIZE, f))
	{
		strip(lbuf);

		replace(lbuf, BUFSIZE, "&network&", me.netname);
		replace(lbuf, BUFSIZE, "&nickexpiry&", nebuf);
		replace(lbuf, BUFSIZE, "&chanexpiry&", cebuf);
		replace(lbuf, BUFSIZE, "&myusers&", ubuf);
		replace(lbuf, BUFSIZE, "&mynicks&", nbuf);
		replace(lbuf, BUFSIZE, "&mychans&", cbuf);

		numeric_sts(me.me, 372, u, ":- %s", lbuf);
	}

	numeric_sts(me.me, 376, u, ":End of the Message of the Day.");

	fclose(f);
}

void
handle_away(struct user *u, const char *message)
{
	if (message == NULL || *message == '\0')
	{
		if (u->flags & UF_AWAY)
		{
			u->flags &= ~UF_AWAY;
			hook_call_user_away(u);
		}
	}
	else
	{
		if (!(u->flags & UF_AWAY))
		{
			u->flags |= UF_AWAY;
			hook_call_user_away(u);
		}
	}
}

static void
handle_channel_message(struct sourceinfo *si, char *target, bool is_notice, char *message)
{
	char *vec[3];
	struct hook_channel_message cdata;
	mowgli_node_t *n, *tn;
	mowgli_list_t l = { NULL, NULL, 0 };
	struct service *svs;

	/* Call hook here */
	cdata.u = si->su;
	cdata.c = channel_find(target);
	cdata.msg = message;

	/* No such channel, ignore... */
	if (cdata.c == NULL)
		return;

	hook_call_channel_message(&cdata);

	vec[0] = target;
	vec[1] = message;
	vec[2] = NULL;

	MOWGLI_ITER_FOREACH(n, cdata.c->members.head)
	{
		struct chanuser *cu = (struct chanuser *) n->data;

		if (!is_internal_client(cu->user))
			continue;

		svs = service_find_nick(cu->user->nick);

		if (svs == NULL)
			continue;

		if (svs->chanmsg == false)
			continue;

		mowgli_node_add(svs, mowgli_node_create(), &l);
	}
	/* Note: this assumes a fantasy command will not remove another
	 * service.
	 */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, l.head)
	{
		si->service = n->data;
		if (is_notice)
			si->service->notice_handler(si, 2, vec);
		else
			si->service->handler(si, 2, vec);
		mowgli_node_delete(n, &l);
		mowgli_node_free(n);
	}
}

void
handle_message(struct sourceinfo *si, char *target, bool is_notice, char *message)
{
	char *vec[3];
	struct user *u, *target_u;
	char *p;
	char name2[NICKLEN + 1];
	char *sentinel;
	mowgli_node_t *n;

	/* message from server, ignore */
	if (si->su == NULL)
		return;

	/* if this is a channel, handle it differently. */
	if (*target == '#')
	{
		handle_channel_message(si, target, is_notice, message);
		return;
	}

	target_u = NULL;
	si->service = NULL;
	p = strchr(target, '@');
	if (p != NULL)
	{
		/* Make sure it's for us, not for a jupe -- jilles */
		if (!irccasecmp(p + 1, me.name))
		{
			mowgli_strlcpy(name2, target, sizeof name2);
			p = strchr(name2, '@');
			if (p != NULL)
				*p = '\0';
			si->service = service_find_nick(name2);
			if (si->service == NULL)
			{
				target_u = NULL;
				MOWGLI_ITER_FOREACH(n, me.me->userlist.head)
				{
					u = n->data;
					/* don't leak info about enforcers */
					if (u->flags & UF_ENFORCER)
						continue;
					if (irccasecmp(u->user, name2))
						continue;
					if (target_u == NULL)
						target_u = u;
					else
					{
						numeric_sts(me.me, 407, si->su, "%s :Ambiguous recipient", target);
						return;
					}
				}
				if (target_u != NULL)
					si->service = service_find_nick(target_u->nick);
			}
		}
	}
	else
	{
		target_u = user_find(target);
		if (target_u != NULL)
			si->service = service_find_nick(target_u->nick);
	}

	if (si->service == NULL)
	{
		if (!is_notice && target_u != NULL && target_u->server == me.me)
		{
			notice(target_u->nick, si->su->nick, "This is a registered nick enforcer, and not a real user.");
			return;
		}
		if (!is_notice && (isalnum((unsigned char)target[0]) || strchr("[\\]^_`{|}~", target[0])))
		{
			/* If it's not a notice and looks like a nick or
			 * user@server, send back an error message */
			if (strchr(target, '@') || !ircd->uses_uid || (!ircd->uses_p10 && !isdigit((unsigned char)target[0])))
				numeric_sts(me.me, 401, si->su, "%s :No such nick", target);
			else
				numeric_sts(me.me, 401, si->su, "* :Target left IRC. Failed to deliver: [%.20s]", message);
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
		notice(si->service->me->nick, si->su->nick, "For security reasons, \2/msg %s\2 has been disabled. "
		       "Use \2/msg %s <command>\2 to send a command.", si->service->me->nick, si->service->disp);
		return;
	}

	/* strip OTR tagging (github issue #8) */
	sentinel = strstr(message, "\x20\x09\x20\x20\x09\x09\x09\x09");
	if (sentinel != NULL)
		*sentinel = '\0';

	vec[0] = target;
	vec[1] = message;
	vec[2] = NULL;
	if (is_notice)
		si->service->notice_handler(si, 2, vec);
	else
		si->service->handler(si, 2, vec);
}

void
handle_topic_from(struct sourceinfo *si, struct channel *c, const char *setter, time_t ts, const char *topic)
{
	struct hook_channel_topic_check hdata;

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
		hook_call_channel_can_change_topic(&hdata);
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
			topic_sts(c, chansvs.me->me, setter, ts, ts, hdata.topic);
		}
	}
	else
	{
		/* Not allowed, change it back */
		if (c->topic != NULL)
			topic_sts(c, chansvs.me->me, c->topic_setter, c->topicts, ts, c->topic);
		else
		{
			/* Ick, there was no topic */
			topic_sts(c, chansvs.me->me, chansvs.nick != NULL ? chansvs.nick : me.name, ts - 1, ts, "");
		}
	}
}

void
handle_topic(struct channel *c, const char *setter, time_t ts, const char *topic)
{
	char newsetter[HOSTLEN + 1], *p;

	/* setter can be a nick, nick!user@host or server.
	 * strip off !user@host part if it's there
	 * (do we really want this?) */
	mowgli_strlcpy(newsetter, setter, sizeof newsetter);
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

	sfree(c->topic);
	sfree(c->topic_setter);

	if (topic != NULL && topic[0] != '\0')
	{
		c->topic = sstrdup(topic);
		c->topic_setter = sstrdup(newsetter);
	}
	else
	{
		c->topic = NULL;
		c->topic_setter = NULL;
	}

	c->topicts = ts;

	hook_call_channel_topic(c);
}

static const char *
skip_kill_path(const char *reason)
{
	const char *p;
	bool have_dot = false;

	p = reason;
	while (*p != ' ')
	{
		if (strchr("<>()#&?%$,;", *p) || *p == '\0')
			return reason;
		if (*p == '!' || *p == '.')
			have_dot = true;
		p++;
	}
	return have_dot ? p + 1 : reason;
}

void
handle_kill(struct sourceinfo *si, const char *victim, const char *reason)
{
	const char *source, *source1, *origreason;
	char qreason[512];
	struct user *u;
	static time_t lastkill = 0;
	static unsigned int killcount = 0;

	source = get_oper_name(si);
	if (si->su)
		source1 = si->su->nick;
	else if (si->s)
		source1 = si->s->name;
	else
		source1 = me.name;
	origreason = reason;
	reason = skip_kill_path(reason);
	if (reason[0] == '[' || !strncmp(reason, "Killed", 6))
		snprintf(qreason, sizeof qreason, "%s", reason);
	else if (origreason == reason)
		snprintf(qreason, sizeof qreason, "Killed (%s (%s))",
				source1, reason);
	else
		snprintf(qreason, sizeof qreason, "Killed (%s %s)",
				source1, reason);

	u = user_find(victim);
	if (u == NULL)
		slog(LG_DEBUG, "handle_kill(): %s killed unknown user %s (%s)", source, victim, reason);
	else if (u->flags & UF_ENFORCER)
	{
		slog(LG_INFO, "handle_kill(): %s killed enforcer %s (%s)", source, u->nick, reason);
		user_delete(u, qreason);
	}
	else if (u->server == me.me)
	{
		// Can't reliably use slog() here, in case it's OperServ (source of message) that got killed!

		wallops("%s killed service %s (%s)", source, u->nick, reason);
		if (lastkill != CURRTIME && killcount < 5 + me.me->users)
		{
			killcount = 0;
			lastkill = CURRTIME;
		}
		killcount++;
		if (killcount < 5 + me.me->users)
			reintroduce_user(u);
		else
		{
			wallops("Services kill fight (%s -> %s), shutting down!", source, u->nick);
			runflags |= RF_SHUTDOWN;
		}
	}
	else
	{
		slog(LG_DEBUG, "handle_kill(): %s killed user %s (%s)", source, u->nick, reason);
		user_delete(u, qreason);
	}
}

struct server *
handle_server(struct sourceinfo *si, const char *name, const char *sid,
		int hops, const char *desc)
{
	struct server *s = NULL;

	if (si->s != NULL)
	{
		/* A server introducing another server */
		s = server_add(name, hops, si->s, sid, desc);
	}
	else if (cnt.server == 1)
	{
		/* Our uplink introducing itself */
		if (irccasecmp(name, curr_uplink->name))
			slog(LG_ERROR, "handle_server(): uplink %s actually has name %s, continuing anyway", curr_uplink->name, name);
		s = server_add(name, hops, me.me, sid, desc);
		me.actual = s->name;
		me.recvsvr = true;
	}
	else
		slog(LG_ERROR, "handle_server(): unregistered/unknown server attempting to introduce another server %s", name);
	return s;
}

void
handle_eob(struct server *s)
{
	mowgli_node_t *n;
	struct server *s2;

	if (s == NULL)
		return;
	if (s->flags & SF_EOB)
		return;
	slog(LG_NETWORK, "handle_eob(): end of burst from %s (%u users)",
			s->name, s->users);
	hook_call_server_eob(s);
	s->flags |= SF_EOB;
	/* convert P10 style EOB to ircnet/ratbox style */
	MOWGLI_ITER_FOREACH(n, s->children.head)
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
int
floodcheck(struct user *u, struct user *t)
{
	const char *from;
	static time_t last_ignore_notice = 0;
	unsigned int reduction;

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
	if (svsignore_find(u) && !has_priv_user(u, PRIV_ADMIN))
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

		if (u->lastmsg != 0 && CURRTIME > u->lastmsg)
		{
			reduction = (CURRTIME - u->lastmsg) * (config_options.flood_msgs * FLOOD_MSGS_FACTOR / config_options.flood_time);
			if (u->msgs > reduction)
				u->msgs -= reduction;
			else
				u->msgs = 0;
		}
		u->lastmsg = CURRTIME;
		u->msgs += FLOOD_MSGS_FACTOR;

		if (u->msgs > config_options.flood_msgs * FLOOD_MSGS_FACTOR)
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

				notice(from, u->nick, _("You have triggered services flood protection."));
				notice(from, u->nick, _("This is your first offense. You will be ignored for 30 seconds."));

				slog(LG_INFO, "FLOOD: \2%s\2", u->nick);

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

				slog(LG_INFO, "FLOOD: \2%s\2", u->nick);

				return 1;
			}

			if (u->offenses == 2)
			{
				kline_add_user(u, "ten minute ban - flooding services", 600, chansvs.nick);

				slog(LG_INFO, "FLOOD:KLINE: \2%s\2", u->nick);

				return 1;
			}
		}
	}

	return 0;
}

void
command_add_flood(struct sourceinfo *si, unsigned int amount)
{
	if (si->su != NULL)
		si->su->msgs += amount;
}

bool
should_reg_umode(struct user *u)
{
	struct mynick *mn;

	if (nicksvs.me == NULL || nicksvs.no_nick_ownership ||
			u->myuser == NULL || u->myuser->flags & MU_WAITAUTH)
		return false;
	mn = mynick_find(u->nick);
	return mn != NULL && mn->owner == u->myuser;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
