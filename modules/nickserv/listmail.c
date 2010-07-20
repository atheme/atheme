/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LISTMAIL function.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/listmail", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_listmail(sourceinfo_t *si, int parc, char *parv[]);

cmd_t ns_listmail = { N_("Lists accounts registered to an e-mail address."), PRIV_USER_AUSPEX, "help/nickserv/listmail", 1, ns_cmd_listmail };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	cmd_add("nickserv:listmail", &ns_listmail);
}

void _moddeinit()
{
	cmd_del("nickserv:listmail");
}

struct listmail_state
{
	sourceinfo_t *origin;
	char *pattern;
	int matches;
};

static int listmail_foreach_cb(const char *key, void *data, void *privdata)
{
	struct listmail_state *state = (struct listmail_state *) privdata;
	myuser_t *mu = (myuser_t *)data;

	if (!match(state->pattern, mu->email))
	{
		/* in the future we could add a LIMIT parameter */
		if (state->matches == 0)
			command_success_nodata(state->origin, "Accounts matching e-mail address \2%s\2:", state->pattern);

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
		command_fail(si, fault_needmoreparams, _("Syntax: LISTMAIL <email>"));
		return;
	}

	state.matches = 0;
	state.pattern = email;
	state.origin = si;
	mowgli_patricia_foreach(mulist, listmail_foreach_cb, &state);

	logcommand(si, CMDLOG_ADMIN, "LISTMAIL: \2%s\2 (\2%d\2 matches)", email, state.matches);
	if (state.matches == 0)
		command_success_nodata(si, _("No accounts matched e-mail address \2%s\2"), email);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 match for e-mail address \2%s\2"),
						    N_("\2%d\2 matches for e-mail address \2%s\2"), state.matches), state.matches, email);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
