/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows setting a vhost on an account
 *
 */

#include "atheme.h"
#include "hostserv.h"

DECLARE_MODULE_V1
(
	"hostserv/vhost", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void hs_cmd_vhost(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_listvhost(sourceinfo_t *si, int parc, char *parv[]);

command_t hs_vhost = { "VHOST", N_("Manages per-account virtual hosts."), PRIV_USER_VHOST, 2, hs_cmd_vhost, { .path = "hostserv/vhost" } };
command_t hs_listvhost = { "LISTVHOST", N_("Lists user virtual hosts."), PRIV_USER_AUSPEX, 1, hs_cmd_listvhost, { .path = "hostserv/listvhost" } };

void _modinit(module_t *m)
{
	service_named_bind_command("hostserv", &hs_vhost);
	service_named_bind_command("hostserv", &hs_listvhost);
}

void _moddeinit(void)
{
	service_named_unbind_command("hostserv", &hs_vhost);
	service_named_unbind_command("hostserv", &hs_listvhost);
}

static void hs_sethost_all(myuser_t *mu, const char *host)
{
	mowgli_node_t *n;
	mynick_t *mn;
	char buf[BUFSIZE];

	MOWGLI_ITER_FOREACH(n, mu->nicks.head)
	{
		mn = n->data;
		snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", mn->nick);
		metadata_delete(mu, buf);
	}
	if (host != NULL)
		metadata_add(mu, "private:usercloak", host);
	else
		metadata_delete(mu, "private:usercloak");
}

/* VHOST <nick> [host] */
static void hs_cmd_vhost(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *host = parv[1];
	myuser_t *mu;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "VHOST");
		command_fail(si, fault_needmoreparams, _("Syntax: VHOST <nick> [vhost]"));
		return;
	}

	/* find the user... */
	if (!(mu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
		return;
	}

	/* deletion action */
	if (!host)
	{
		hs_sethost_all(mu, NULL);
		command_success_nodata(si, _("Deleted all vhosts for \2%s\2."), entity(mu)->name);
		logcommand(si, CMDLOG_ADMIN, "VHOST:REMOVE: \2%s\2", target);
		do_sethost_all(mu, NULL); // restore user vhost from user host
		return;
	}

	if (!check_vhost_validity(si, host))
		return;

	hs_sethost_all(mu, host);
	command_success_nodata(si, _("Assigned vhost \2%s\2 to all nicks in account \2%s\2."),
			host, entity(mu)->name);
	logcommand(si, CMDLOG_ADMIN, "VHOST:ASSIGN: \2%s\2 to \2%s\2",
			host, target);
	do_sethost_all(mu, host);
	return;
}

static void hs_cmd_listvhost(sourceinfo_t *si, int parc, char *parv[])
{
	const char *pattern;
	myentity_iteration_state_t state;
	myentity_t *mt;
	myuser_t *mu;
	metadata_t *md;
	mowgli_node_t *n;
	char buf[BUFSIZE];
	int matches = 0;

	pattern = parc >= 1 ? parv[0] : "*";

	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		mu = user(mt);
		md = metadata_find(mu, "private:usercloak");
		if (md != NULL && !match(pattern, md->value))
		{
			command_success_nodata(si, "- %-30s %s", entity(mu)->name, md->value);
			matches++;
		}
		MOWGLI_ITER_FOREACH(n, mu->nicks.head)
		{
			snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", ((mynick_t *)(n->data))->nick);
			md = metadata_find(mu, buf);
			if (md == NULL)
				continue;
			if (!match(pattern, md->value))
			{
				command_success_nodata(si, "- %-30s %s", ((mynick_t *)(n->data))->nick, md->value);
				matches++;
			}
		}
	}

	logcommand(si, CMDLOG_ADMIN, "LISTVHOST: \2%s\2 (\2%d\2 matches)", pattern, matches);
	if (matches == 0)
		command_success_nodata(si, _("No vhosts matched pattern \2%s\2"), pattern);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 match for pattern \2%s\2"),
						    N_("\2%d\2 matches for pattern \2%s\2"), matches), matches, pattern);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
