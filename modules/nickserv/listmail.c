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
	"nickserv/listmail", FALSE, _modinit, _moddeinit,
	"$Id: listmail.c 5981 2006-07-31 22:33:14Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_listmail(char *origin);

command_t ns_listmail = { "LISTMAIL", "Lists nicknames registered to an e-mail address.", PRIV_USER_AUSPEX, ns_cmd_listmail };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_listmail, ns_cmdtree);
	help_addentry(ns_helptree, "LISTMAIL", "help/nickserv/listmail", NULL);
}

void _moddeinit()
{
	command_delete(&ns_listmail, ns_cmdtree);
	help_delentry(ns_helptree, "LISTMAIL");
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
			notice(nicksvs.nick, state->origin, "Nicknames matching e-mail address \2%s\2:", state->pattern);

		notice(nicksvs.nick, state->origin, "- %s (%s)", mu->name, mu->email);
		state->matches++;
	}

	return 0;
}

static void ns_cmd_listmail(char *origin)
{
	user_t *u = user_find_named(origin);
	char *email = strtok(NULL, " ");
	struct listmail_state state;

	if (u == NULL)
		return;

	if (!email)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "LISTMAIL");
		notice(nicksvs.nick, origin, "Syntax: LISTMAIL <email>");
		return;
	}

	wallops("\2%s\2 is searching the nickname database for e-mail address \2%s\2", origin, email);

	state.matches = 0;
	state.origin = origin;
	state.pattern = email;
	dictionary_foreach(mulist, listmail_foreach_cb, &state);

	logcommand(nicksvs.me, u, CMDLOG_ADMIN, "LISTMAIL %s (%d matches)", email, state.matches);
	if (state.matches == 0)
		notice(nicksvs.nick, origin, "No nicknames matched e-mail address \2%s\2", email);
	else
		notice(nicksvs.nick, origin, "\2%d\2 match%s for e-mail address \2%s\2", state.matches, state.matches != 1 ? "es" : "", email);
}
