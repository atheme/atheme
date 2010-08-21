/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows syncing the vhost for all nicks in a group.
 *
 */

#include "atheme.h"
#include "hostserv.h"

DECLARE_MODULE_V1
(
	"hostserv/group", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *hs_cmdtree, *hs_helptree;

static void hs_cmd_group(sourceinfo_t *si, int parc, char *parv[]);

command_t hs_group = { "GROUP", N_("Syncs the vhost for all nicks in a group."), AC_NONE, 1, hs_cmd_group };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(hs_cmdtree, "hostserv/main", "hs_cmdtree");
	MODULE_USE_SYMBOL(hs_helptree, "hostserv/main", "hs_helptree");

	command_add(&hs_group, hs_cmdtree);
	help_addentry(hs_helptree, "GROUP", "help/hostserv/group", NULL);
}

void _moddeinit(void)
{
	command_delete(&hs_group, hs_cmdtree);
	help_delentry(hs_helptree, "GROUP");
}

static void hs_sethost_all(myuser_t *mu, const char *host)
{
	node_t *n;
	mynick_t *mn;
	char buf[BUFSIZE];

	LIST_FOREACH(n, mu->nicks.head)
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

static void hs_cmd_group(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn;
	metadata_t *md;
	char buf[BUFSIZE];

	/* This is only because we need a nick to copy from. */
	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), "GROUP");
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}
	mn = mynick_find(si->su->nick);
	if (mn == NULL)
	{
		command_fail(si, fault_nosuch_target, _("Nick \2%s\2 is not registered."), si->su->nick);
		return;
	}
	if (mn->owner != si->smu)
	{
		command_fail(si, fault_noprivs, _("Nick \2%s\2 is not registered to your account."), mn->nick);
		return;
	}
	snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", mn->nick);
	md = metadata_find(si->smu, buf);
	if (md == NULL)
		md = metadata_find(si->smu, "private:usercloak");
	if (md == NULL)
	{
		command_success_nodata(si, _("Please contact an Operator to get a vhost assigned to this nick."));
		return;
	}
	strlcpy(buf, md->value, sizeof buf);
	hs_sethost_all(si->smu, buf);
	do_sethost_all(si->smu, buf);
	command_success_nodata(si, _("All vhosts in the group \2%s\2 have been set to \2%s\2."), entity(si->smu)->name, buf);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
