/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LISTMAIL function.
 *
 * $Id: listmail.c 5981 2006-07-31 22:33:14Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/listmail", FALSE, _modinit, _moddeinit,
	"$Id: listmail.c 5981 2006-07-31 22:33:14Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_listmail(char *origin);

command_t us_listmail = { "LISTMAIL", "Lists accounts registered to an e-mail address.", PRIV_USER_AUSPEX, us_cmd_listmail };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_listmail, us_cmdtree);
	help_addentry(us_helptree, "LISTMAIL", "help/userserv/listmail", NULL);
}

void _moddeinit()
{
	command_delete(&us_listmail, us_cmdtree);
	help_delentry(us_helptree, "LISTMAIL");
}

struct listmail_state
{
	char *origin;
	char *pattern;
	int matches;
};

static int listmail_foreach_cb(dictionary_elem_t *delem, void *privdata)
{
	struct listmail_state *state = (struct listmail_state *) privdata;
	myuser_t *mu = (myuser_t *)delem->node.data;

	if (!match(state->pattern, mu->email))
	{
		/* in the future we could add a LIMIT parameter */
		if (state->matches == 0)
			notice(usersvs.nick, state->origin, "Accounts matching e-mail address \2%s\2:", state->pattern);

		notice(usersvs.nick, state->origin, "- %s (%s)", mu->name, mu->email);
		state->matches++;
	}

	return 0;
}

static void us_cmd_listmail(char *origin)
{
	user_t *u = user_find_named(origin);
	char *email = strtok(NULL, " ");
	struct listmail_state state;

	if (u == NULL)
		return;

	if (!email)
	{
		notice(usersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "LISTMAIL");
		notice(usersvs.nick, origin, "Syntax: LISTMAIL <email>");
		return;
	}

	wallops("\2%s\2 is searching the account database for e-mail address \2%s\2", origin, email);

	state.matches = 0;
	state.pattern = email;
	state.origin = origin;
	dictionary_foreach(mulist, listmail_foreach_cb, &state);

	logcommand(usersvs.me, u, CMDLOG_ADMIN, "LISTMAIL %s (%d matches)", email, state.matches);
	if (state.matches == 0)
		notice(usersvs.nick, origin, "No accounts matched e-mail address \2%s\2", email);
	else
		notice(usersvs.nick, origin, "\2%d\2 match%s for e-mail address \2%s\2", state.matches, state.matches != 1 ? "es" : "", email);
}
