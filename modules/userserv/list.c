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
	"userserv/list", FALSE, _modinit, _moddeinit,
	"$Id: list.c 6205 2006-08-21 14:36:38Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_list(char *origin);

command_t us_list = { "LIST", "Lists accounts registered matching a given pattern.", PRIV_USER_AUSPEX, us_cmd_list };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_list, us_cmdtree);
	help_addentry(us_helptree, "LIST", "help/userserv/list", NULL);
}

void _moddeinit()
{
	command_delete(&us_list, us_cmdtree);
	help_delentry(us_helptree, "LIST");
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
				
		notice(usersvs.nick, state->origin, "- %s (%s) %s", mu->name, mu->email, buf);
		state->matches++;
	}
}

static void us_cmd_list(char *origin)
{
	user_t *u = user_find_named(origin);
	char *nickpattern = strtok(NULL, " ");
	uint32_t i;
	struct list_state state;

	if (u == NULL)
		return;

	if (!nickpattern)
	{
		notice(usersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "LIST");
		notice(usersvs.nick, origin, "Syntax: LIST <account pattern>");
		return;
	}

	snoop("LIST:ACCOUNTS: \2%s\2 by \2%s\2", nickpattern, origin);

	state.origin = origin;
	state.pattern = nickpattern;
	state.matches = 0;
	dictionary_foreach(mulist, list_foreach_cb, &state);

	logcommand(usersvs.me, u, CMDLOG_ADMIN, "LIST %s (%d matches)", nickpattern, state.matches);
	if (state.matches == 0)
		notice(usersvs.nick, origin, "No accounts matched pattern \2%s\2", nickpattern);
	else
		notice(usersvs.nick, origin, "\2%d\2 match%s for pattern \2%s\2", state.matches, state.matches != 1 ? "es" : "", nickpattern);
}
