/*
 * Copyright (c) 2005-2006 Robin Burchell, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LIST function.
 * Based on Alex Lambert's LISTEMAIL.
 *
 * $Id: list.c 7289 2006-11-25 19:18:57Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/list", FALSE, _modinit, _moddeinit,
	"$Id: list.c 7289 2006-11-25 19:18:57Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_list(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_list = { "LIST", "Lists nicknames registered matching a given pattern.", PRIV_USER_AUSPEX, 1, ns_cmd_list };

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

static int list_one(sourceinfo_t *si, myuser_t *mu, mynick_t *mn)
{
	char buf[BUFSIZE];

	if (mn != NULL)
		mu = mn->owner;

	*buf = '\0';
	if (metadata_find(mu, METADATA_USER, "private:freeze:freezer")) {
		if (*buf)
			strlcat(buf, " ", BUFSIZE);

		strlcat(buf, "\2[frozen]\2", BUFSIZE);
	}
	if (metadata_find(mu, METADATA_USER, "private:mark:setter")) {
		if (*buf)
			strlcat(buf, " ", BUFSIZE);

		strlcat(buf, "\2[marked]\2", BUFSIZE);
	}

	if (mn == NULL || !irccasecmp(mn->nick, mu->name))
		command_success_nodata(si, "- %s (%s) %s", mu->name, mu->email, buf);
	else
		command_success_nodata(si, "- %s (%s) (%s) %s", mn->nick, mu->email, mu->name, buf);
}

static void ns_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	char *nickpattern = parv[0];
	dictionary_iteration_state_t state;
	myuser_t *mu;
	mynick_t *mn;
	int matches = 0;

	if (!nickpattern)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LIST");
		command_fail(si, fault_needmoreparams, "Syntax: LIST <pattern>");
		return;
	}

	if (nicksvs.no_nick_ownership)
	{
		snoop("LIST:ACCOUNTS: \2%s\2 by \2%s\2", nickpattern, get_oper_name(si));
		DICTIONARY_FOREACH(mu, &state, mulist)
		{
			if (!match(nickpattern, mu->name))
			{
				list_one(si, mu, NULL);
				matches++;
			}
		}
	}
	else
	{
		snoop("LIST:NICKS: \2%s\2 by \2%s\2", nickpattern, get_oper_name(si));
		DICTIONARY_FOREACH(mn, &state, nicklist)
		{
			if (!match(nickpattern, mn->nick))
			{
				list_one(si, NULL, mn);
				matches++;
			}
		}
	}

	logcommand(si, CMDLOG_ADMIN, "LIST %s (%d matches)", nickpattern, matches);
	if (matches == 0)
		command_success_nodata(si, "No nicknames matched pattern \2%s\2", nickpattern);
	else
		command_success_nodata(si, "\2%d\2 match%s for pattern \2%s\2", matches, matches != 1 ? "es" : "", nickpattern);
}
