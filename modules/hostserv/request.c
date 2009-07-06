/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows request a vhost on a nick
 *
 * $Id: request.c 8195 2007-04-25 16:27:08Z celestin $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"hostserv/request", false, _modinit, _moddeinit,
	"$Id: vhost.c 8195 2007-04-25 16:27:08Z celestin $",
	"Rizon Development Group <http://www.rizon.net>"
);

list_t *hs_cmdtree, *hs_helptree, *ms_cmdtree;

static void account_drop_request(void *vptr);
static void nick_drop_request(void *vptr);
static void hs_cmd_request(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_waiting(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_reject(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_activate(sourceinfo_t *si, int parc, char *parv[]);
static void write_hsreqdb(void);
static void load_hsreqdb(void);

command_t hs_request = { "REQUEST", N_("Requests new virtual hostname for current nick."), AC_NONE, 2, hs_cmd_request };
command_t hs_waiting = { "WAITING", N_("Activates your assigned vhost."), PRIV_USER_VHOST, 1, hs_cmd_waiting };
command_t hs_reject = { "REJECT", N_("Deactivates your assigned vhost."), PRIV_USER_VHOST, 2, hs_cmd_reject };
command_t hs_activate = { "ACTIVATE", N_("Syncs the vhost for all nicks in a group."), PRIV_USER_VHOST, 2, hs_cmd_activate };

struct hsreq_ {
	char *nick;
	char *vident;
	char *vhost;
	time_t vhost_ts;
	char *creator;
};

typedef struct hsreq_ hsreq_t;

list_t hs_reqlist;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(hs_cmdtree, "hostserv/main", "hs_cmdtree");
	MODULE_USE_SYMBOL(hs_helptree, "hostserv/main", "hs_helptree");
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");
	MODULE_TRY_REQUEST_DEPENDENCY(m, "hostserv/vhost");

	hook_add_event("user_drop");
	hook_add_hook("user_drop", account_drop_request);
	hook_add_event("nick_ungroup");
	hook_add_hook("nick_ungroup", nick_drop_request);
 	command_add(&hs_request, hs_cmdtree);
	command_add(&hs_waiting, hs_cmdtree);
	command_add(&hs_reject, hs_cmdtree);
	command_add(&hs_activate, hs_cmdtree);
	help_addentry(hs_helptree, "REQUEST", "help/hostserv/request", NULL);
	help_addentry(hs_helptree, "WAITING", "help/hostserv/waiting", NULL);
	help_addentry(hs_helptree, "REJECT", "help/hostserv/reject", NULL);
	help_addentry(hs_helptree, "ACTIVATE", "help/hostserv/activate", NULL);
	load_hsreqdb();
}

void _moddeinit(void)
{
	hook_del_hook("user_drop", account_drop_request);
	hook_del_hook("nick_ungroup", nick_drop_request);
	command_delete(&hs_request, hs_cmdtree);
	command_delete(&hs_waiting, hs_cmdtree);
	command_delete(&hs_reject, hs_cmdtree);
	command_delete(&hs_activate, hs_cmdtree);
	help_delentry(hs_helptree, "REQUEST");
	help_delentry(hs_helptree, "WAITING");
	help_delentry(hs_helptree, "REJECT");
	help_delentry(hs_helptree, "ACTIVATE");
}

static void write_hsreqdb(void)
{
	FILE *f;
	node_t *n;
	hsreq_t *l;

	if (!(f = fopen(DATADIR "/hsreq.db.new", "w")))
	{
		slog(LG_DEBUG, "write_hsreqdb(): cannot write vhost requests database: %s", strerror(errno));
		return;
	}

	LIST_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;
		fprintf(f, "HR %s %s %s %ld %s\n", l->nick, l->vident, l->vhost,
			(long)l->vhost_ts, l->creator);
	}

	fclose(f);

	if ((rename(DATADIR "/hsreq.db.new", DATADIR "/hsreq.db")) < 0)
	{
		slog(LG_DEBUG, "write_hsreqdb(): couldn't rename vhost requests database.");
		return;
	}
}

static void load_hsreqdb(void)
{
	FILE *f;
	node_t *n;
	hsreq_t *l;
	char *item, rBuf[BUFSIZE];

	if (!(f = fopen(DATADIR "/hsreq.db", "r")))
	{
		slog(LG_DEBUG, "load_hsreqdb(): cannot open vhost requests database: %s", strerror(errno));
		return;
	}

	while (fgets(rBuf, BUFSIZE, f))
	{
		item = strtok(rBuf, " ");
		strip(item);

		if (!strcmp(item, "HR"))
		{
			char *nick = strtok(NULL, " ");
			char *vident = strtok(NULL, " ");
			char *vhost = strtok(NULL, " ");
			char *vhost_ts = strtok(NULL, " ");
			char *creator = strtok(NULL, "\n");

			if (!nick || !vident || !vhost || !vhost_ts || !creator)
				continue;

			l = smalloc(sizeof(hsreq_t));
			l->nick = sstrdup(nick);
			l->vident = sstrdup(vident);
			l->vhost = sstrdup(vhost);
			l->vhost_ts = atol(vhost_ts);
			l->creator = sstrdup(creator);

			n = node_create();
			node_add(l, n, &hs_reqlist);
		}
	}

	fclose(f);
}

static void nick_drop_request(void *vptr)
{
	hook_user_req_t *hdata = vptr;
	node_t *m;
	hsreq_t *l;

	{
		{
			{
				LIST_FOREACH(m, hs_reqlist.head)
				{
					l = m->data;
					if (!irccasecmp(l->nick, hdata->mn->nick))
					{
						slog(LG_REGISTER, "VHOSTREQ DROPNICK %s %s@%s", l->nick, l->vident, l->vhost);

						node_del(m, &hs_reqlist);

						free(l->nick);
						free(l->vident);
						free(l->vhost);
						free(l->creator);
						free(l);

						write_hsreqdb();
						return;
					}
				}
			}
		}
	}
}

static void account_drop_request(void *vptr)
{
	myuser_t *mu = vptr;
	node_t *n;
	hsreq_t *l;

	LIST_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, mu->name))
		{
			slog(LG_REGISTER, "VHOSTREQ DROPACCOUNT %s %s@%s", l->nick, l->vident, l->vhost);

			node_del(n, &hs_reqlist);

			free(l->nick);
			free(l->vident);
			free(l->vhost);
			free(l->creator);
			free(l);

			write_hsreqdb();
			return;
		}
	}
}

/* REQUEST <host> */
static void hs_cmd_request(sourceinfo_t *si, int parc, char *parv[])
{
	char *host = parv[0];
	char *target;
	mynick_t *mn;
	char *p;
	node_t *n;
	hsreq_t *l;

	if (!host)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REQUEST");
		command_fail(si, fault_needmoreparams, _("Syntax: REQUEST <vhost>"));
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}
	target = si->su != NULL ? si->su->nick : "?";
	mn = mynick_find(target);
	if (mn == NULL)
	{
		command_fail(si, fault_nosuch_target, _("Nick \2%s\2 is not registered."), target);
		return;
	}
	if (mn->owner != si->smu)
	{
		command_fail(si, fault_noprivs, _("Nick \2%s\2 is not registered to your account."), mn->nick);
		return;
	}

	/* Never ever allow @!?* as they have special meaning in all ircds */
	/* Empty, space anywhere and colon at the start break the protocol */
	if (strchr(host, '@') || strchr(host, '!') || strchr(host, '?') ||
			strchr(host, '*') || strchr(host, ' ') ||
			*host == ':' || *host == '\0')
	{
		command_fail(si, fault_badparams, _("The vhost provided contains invalid characters."));
		return;
	}
	if (strlen(host) >= HOSTLEN)
	{
		command_fail(si, fault_badparams, _("The vhost provided is too long."));
		return;
	}
	p = strrchr(host, '/');
	if (p != NULL && isdigit(p[1]))
	{
		command_fail(si, fault_badparams, _("The vhost provided looks like a CIDR mask."));
		return;
	}
	if (!is_valid_host(host))
	{
		/* This can be stuff like missing dots too. */
		command_fail(si, fault_badparams, _("The vhost provided is invalid."));
		return;
	}

	/* search for it */
	LIST_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;

		if (!irccasecmp(l->nick, target))
		{
			if (!strcmp(host, l->vhost))
			{
				command_success_nodata(si, _("You have requested vhost \2%s\2."), host);
				return;
			}
			free(l->vhost);
			l->vhost = sstrdup(host);
			l->vhost_ts = CURRTIME;;

			write_hsreqdb();

			command_success_nodata(si, _("You have requested vhost \2%s\2."), host);
			snoop("VHOST:REQUEST: \2%s\2 requested \2%s\2", get_source_name(si), host);
			logcommand(si, CMDLOG_SET, "REQUEST %s", host);
			return;
		}
	}

	l = smalloc(sizeof(hsreq_t));
	l->nick = sstrdup(target);
	l->vident = sstrdup("(null)");
	l->vhost = sstrdup(host);
	l->vhost_ts = CURRTIME;;
	l->creator = sstrdup(get_source_name(si));

	n = node_create();
	node_add(l, n, &hs_reqlist);
	write_hsreqdb();

	command_success_nodata(si, _("You have requested vhost \2%s\2."), host);
	snoop("VHOST:REQUEST: \2%s\2 requested \2%s\2", get_source_name(si), host);
	logcommand(si, CMDLOG_SET, "REQUEST %s", host);

	return;
}

/* ACTIVATE <nick> */
static void hs_cmd_activate(sourceinfo_t *si, int parc, char *parv[])
{
	char *nick = parv[0];
	user_t *u;
	char buf[BUFSIZE];
	hsreq_t *l;
	node_t *n;

	if (!nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACTIVATE");
		command_fail(si, fault_needmoreparams, _("Syntax: ACTIVATE <nick>"));
		return;
	}


	LIST_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, nick))
		{
			if (memosvs.me)
			{
				snprintf(buf, BUFSIZE, "%s [auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been approved.", nick, l->vhost, nick);
				command_exec_split(memosvs.me, si, "SEND", buf, ms_cmdtree);
			}
			else if ((u = user_find_named(nick)) != NULL)
				notice(hostsvs.nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been approved.", l->vhost, nick);
			/* VHOST command below will generate snoop */
			logcommand(si, CMDLOG_ADMIN, "ACTIVATE %s for %s", l->vhost, nick);
			snprintf(buf, BUFSIZE, "%s %s", l->nick, l->vhost);

			node_del(n, &hs_reqlist);

			free(l->nick);
			free(l->vident);
			free(l->vhost);
			free(l->creator);
			free(l);

			write_hsreqdb();
			command_exec_split(hostsvs.me, si, "VHOST", buf, hs_cmdtree);
			return;
		}
	}
	command_success_nodata(si, _("Nick \2%s\2 not found in vhost request database."), nick);
}

/* REJECT <nick> */
static void hs_cmd_reject(sourceinfo_t *si, int parc, char *parv[])
{
	char *nick = parv[0];
	user_t *u;
	char buf[BUFSIZE];
	hsreq_t *l;
	node_t *n;

	if (!nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REJECT");
		command_fail(si, fault_needmoreparams, _("Syntax: REJECT <nick>"));
		return;
	}


	LIST_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, nick))
		{
			if (memosvs.me)
			{
				snprintf(buf, BUFSIZE, "%s [auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected.", nick, l->vhost, nick);
				command_exec_split(memosvs.me, si, "SEND", buf, ms_cmdtree);
			}
			else if ((u = user_find_named(nick)) != NULL)
				notice(hostsvs.nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected.", l->vhost, nick);
			snoop("VHOST:REJECT: \2%s\2 for \2%s\2 by \2%s\2",
					l->vhost, nick, get_oper_name(si));
			logcommand(si, CMDLOG_ADMIN, "REJECT %s for %s", l->vhost, nick);

			node_del(n, &hs_reqlist);
			free(l->nick);
			free(l->vident);
			free(l->vhost);
			free(l->creator);
			free(l);
			write_hsreqdb();
			return;
		}
	}
	command_success_nodata(si, _("Nick \2%s\2 not found in vhost request database."), nick);
}

/* WAITING */
static void hs_cmd_waiting(sourceinfo_t *si, int parc, char *parv[])
{
	hsreq_t *l;
	node_t *n;
	int x = 0;
	char buf[BUFSIZE];
	struct tm tm;

	LIST_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;
		x++;

		tm = *localtime(&l->vhost_ts);
		strftime(buf, BUFSIZE, "%b %d %T %Y %Z", &tm);
		if (!irccasecmp(l->vident, "(null)"))
			command_success_nodata(si, "#%d Nick:\2%s\2, vhost:\2%s\2 (%s - %s)",
				x, l->nick, l->vhost, l->creator, buf);
		else
			command_success_nodata(si, "#%d Nick:\2%s\2, vhost:\2%s@%s\2 (%s - %s)",
				x, l->nick, l->vident, l->vhost, l->creator, buf);
	}
	command_success_nodata(si, "End of list.");
	logcommand(si, CMDLOG_GET, "WAITING");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
