/*
 * Copyright (c) 2005 Robin Burchell, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LIST function.
 * Based on Alex Lambert's LISTEMAIL.
 *
 * $Id: list.c 6205 2006-08-21 14:36:38Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/list", FALSE, _modinit, _moddeinit,
	"$Id: list.c 6205 2006-08-21 14:36:38Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_list(char *origin);

command_t ns_list = { "LIST", "Lists nicknames registered matching a given pattern.", PRIV_USER_AUSPEX, ns_cmd_list };

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

struct list_state
{
	char *origin;
	char *pattern;
	int matches;
};

static int list_foreach_cb(dictionary_elem_t *delem, void *privdata)
{
	struct list_state *state = (struct list_state *) privdata;
	myuser_t *mu = (myuser_t *) delem->node.data;
	char buf[BUFSIZE];

	if (!match(state->pattern, mu->name))
	{
		/* in the future we could add a LIMIT parameter */
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
				
		notice(nicksvs.nick, state->origin, "- %s (%s) %s", mu->name, mu->email, buf);
		state->matches++;
	}
}

static void ns_cmd_list(char *origin)
{
	user_t *u = user_find_named(origin);
	char *nickpattern = strtok(NULL, " ");
	struct list_state state;

	if (u == NULL)
		return;

	if (!nickpattern)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "LIST");
		notice(nicksvs.nick, origin, "Syntax: LIST <nickname pattern>");
		return;
	}

	snoop("LIST:NICKS: \2%s\2 by \2%s\2", nickpattern, origin);
	notice(nicksvs.nick, origin, "Nicknames matching pattern \2%s\2:", nickpattern);

	state.origin = origin;
	state.pattern = nickpattern;
	state.matches = 0;
	dictionary_foreach(mulist, list_foreach_cb, &state);

	logcommand(nicksvs.me, u, CMDLOG_ADMIN, "LIST %s (%d matches)", nickpattern, state.matches);
	if (state.matches == 0)
		notice(nicksvs.nick, origin, "No nicknames matched pattern \2%s\2", nickpattern);
	else
		notice(nicksvs.nick, origin, "\2%d\2 match%s for pattern \2%s\2", state.matches, state.matches != 1 ? "es" : "", nickpattern);
}
