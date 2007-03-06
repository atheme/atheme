/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LISTMAIL function.
 *
 * $Id: listmail.c 7855 2007-03-06 00:43:08Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/listmail", FALSE, _modinit, _moddeinit,
	"$Id: listmail.c 7855 2007-03-06 00:43:08Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_listmail(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_listmail = { "LISTMAIL", N_("Lists nicknames registered to an e-mail address."), PRIV_USER_AUSPEX, 1, ns_cmd_listmail };

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
	sourceinfo_t *origin;
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
			command_success_nodata(state->origin, "Nicknames matching e-mail address \2%s\2:", state->pattern);

		command_success_nodata(state->origin, "- %s (%s)", mu->name, mu->email);
		state->matches++;
	}

	return 0;
}

static void ns_cmd_listmail(sourceinfo_t *si, int parc, char *parv[])
{
	char *email = parv[0];
	struct listmail_state state;

	if (!email)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LISTMAIL");
		command_fail(si, fault_needmoreparams, "Syntax: LISTMAIL <email>");
		return;
	}

	snoop("LISTMAIL: \2%s\2 by \2%s\2", email, get_oper_name(si));

	state.matches = 0;
	state.pattern = email;
	state.origin = si;
	dictionary_foreach(mulist, listmail_foreach_cb, &state);

	logcommand(si, CMDLOG_ADMIN, "LISTMAIL %s (%d matches)", email, state.matches);
	if (state.matches == 0)
		command_success_nodata(si, "No nicknames matched e-mail address \2%s\2", email);
	else
		command_success_nodata(si, "\2%d\2 match%s for e-mail address \2%s\2", state.matches, state.matches != 1 ? "es" : "", email);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
