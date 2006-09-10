/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LISTMAIL function.
 *
 * $Id: listmail.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/listmail", FALSE, _modinit, _moddeinit,
	"$Id: listmail.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_listmail(sourceinfo_t *si, int parc, char *parv[]);

command_t us_listmail = { "LISTMAIL", "Lists accounts registered to an e-mail address.", PRIV_USER_AUSPEX, 1, us_cmd_listmail };

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

static void us_cmd_listmail(sourceinfo_t *si, int parc, char *parv[])
{
	char *email = parv[0];
	struct listmail_state state;

	if (!email)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "LISTMAIL");
		notice(usersvs.nick, si->su->nick, "Syntax: LISTMAIL <email>");
		return;
	}

	snoop("LISTMAIL: \2%s\2 by \2%s\2", email, si->su->nick);

	state.matches = 0;
	state.pattern = email;
	state.origin = si->su->nick;
	dictionary_foreach(mulist, listmail_foreach_cb, &state);

	logcommand(usersvs.me, si->su, CMDLOG_ADMIN, "LISTMAIL %s (%d matches)", email, state.matches);
	if (state.matches == 0)
		notice(usersvs.nick, si->su->nick, "No accounts matched e-mail address \2%s\2", email);
	else
		notice(usersvs.nick, si->su->nick, "\2%d\2 match%s for e-mail address \2%s\2", state.matches, state.matches != 1 ? "es" : "", email);
}
