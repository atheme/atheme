/*
 * Copyright (c) 2005-2007 Robin Burchell, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LIST function.
 * Based on Alex Lambert's LISTEMAIL.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/list", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_list(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_list = { "LIST", N_("Lists nicknames registered matching a given pattern."), PRIV_USER_AUSPEX, 1, ns_cmd_list };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_list, ns_cmdtree);
	help_addentry(ns_helptree, "LIST", "help/nickserv/list", NULL);
}

void _moddeinit()
{
	command_delete(&ns_list, ns_cmdtree);
	help_delentry(ns_helptree, "LIST");
}

static void list_one(sourceinfo_t *si, myuser_t *mu, mynick_t *mn)
{
	char buf[BUFSIZE];

	if (mn != NULL)
		mu = mn->owner;

	*buf = '\0';
	if (metadata_find(mu, "private:freeze:freezer")) {
		if (*buf)
			strlcat(buf, " ", BUFSIZE);

		strlcat(buf, "\2[frozen]\2", BUFSIZE);
	}
	if (metadata_find(mu, "private:mark:setter")) {
		if (*buf)
			strlcat(buf, " ", BUFSIZE);

		strlcat(buf, "\2[marked]\2", BUFSIZE);
	}
	if (mu->flags & MU_HOLD) {
		if (*buf)
			strlcat(buf, " ", BUFSIZE);

		strlcat(buf, "\2[held]\2", BUFSIZE);
	}
	if (mu->flags & MU_WAITAUTH) {
		if (*buf)
			strlcat(buf, " ", BUFSIZE);

		strlcat(buf, "\2[unverified]\2", BUFSIZE);
	}

	if (mn == NULL || !irccasecmp(mn->nick, entity(mu)->name))
		command_success_nodata(si, "- %s (%s) %s", entity(mu)->name, mu->email, buf);
	else
		command_success_nodata(si, "- %s (%s) (%s) %s", mn->nick, mu->email, entity(mu)->name, buf);
}

static void ns_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	char pat[512], *nickpattern = NULL, *hostpattern = NULL, *p;
	bool hostmatch;
	mowgli_patricia_iteration_state_t state;
	myentity_iteration_state_t mestate;
	myuser_t *mu;
	myentity_t *mt;
	mynick_t *mn;
	metadata_t *md;
	int matches = 0;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LIST");
		command_fail(si, fault_needmoreparams, _("Syntax: LIST <pattern>[!<user@host>]"));
		return;
	}
	strlcpy(pat, parv[0], sizeof pat);
	p = strrchr(pat, ' ');
	if (p == NULL)
		p = strrchr(pat, '!');
	if (p != NULL)
	{
		*p++ = '\0';
		nickpattern = pat;
		hostpattern = p;
	}
	else if (strchr(pat, '@'))
		hostpattern = pat;
	else
		nickpattern = pat;
	if (nickpattern && !strcmp(nickpattern, "*"))
		nickpattern = NULL;

	if (nicksvs.no_nick_ownership)
	{
		MYENTITY_FOREACH_T(mt, &mestate, ENT_USER)
		{
			mu = user(mt);
			if (nickpattern && match(nickpattern, entity(mu)->name))
				continue;
			if (hostpattern)
			{
				hostmatch = false;
				md = metadata_find(mu, "private:host:actual");
				if (md != NULL && !match(hostpattern, md->value))
					hostmatch = true;
				md = metadata_find(mu, "private:host:vhost");
				if (md != NULL && !match(hostpattern, md->value))
					hostmatch = true;
				if (!hostmatch)
					continue;
			}
			list_one(si, mu, NULL);
			matches++;
		}
	}
	else
	{
		MOWGLI_PATRICIA_FOREACH(mn, &state, nicklist)
		{
			if (nickpattern && match(nickpattern, mn->nick))
				continue;
			mu = mn->owner;
			if (hostpattern)
			{
				hostmatch = false;
				md = metadata_find(mu, "private:host:actual");
				if (md != NULL && !match(hostpattern, md->value))
					hostmatch = true;
				md = metadata_find(mu, "private:host:vhost");
				if (md != NULL && !match(hostpattern, md->value))
					hostmatch = true;
				if (!hostmatch)
					continue;
			}
			list_one(si, NULL, mn);
			matches++;
		}
	}

	logcommand(si, CMDLOG_ADMIN, "LIST: \2%s\2 (\2%d\2 matches)", parv[0], matches);
	if (matches == 0)
		command_success_nodata(si, _("No nicknames matched pattern \2%s\2"), parv[0]);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 match for pattern \2%s\2"), N_("\2%d\2 matches for pattern \2%s\2"), matches), matches, parv[0]);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
