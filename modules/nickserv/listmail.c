/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LISTMAIL function.
 */

#include "atheme.h"

static void ns_cmd_listmail(struct sourceinfo *si, int parc, char *parv[]);

static struct command ns_listmail = { "LISTMAIL", N_("Lists accounts registered to an e-mail address."), PRIV_USER_AUSPEX, 1, ns_cmd_listmail, { .path = "nickserv/listmail" } };

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("nickserv", &ns_listmail);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_listmail);
}

struct listmail_state
{
	struct sourceinfo *origin;
	char *pattern;
	stringref email_canonical;
	int matches;
};

static int
listmail_foreach_cb(struct myentity *mt, void *privdata)
{
	struct listmail_state *state = (struct listmail_state *) privdata;
	struct myuser *mu = user(mt);

	if (state->email_canonical == mu->email_canonical || !match(state->pattern, mu->email))
	{
		/* in the future we could add a LIMIT parameter */
		if (state->matches == 0)
			command_success_nodata(state->origin, "Accounts matching e-mail address \2%s\2:", state->pattern);

		command_success_nodata(state->origin, "- %s (%s)", entity(mu)->name, mu->email);
		state->matches++;
	}

	return 0;
}

static void
ns_cmd_listmail(struct sourceinfo *si, int parc, char *parv[])
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
	state.email_canonical = canonicalize_email(email);
	state.origin = si;
	myentity_foreach_t(ENT_USER, listmail_foreach_cb, &state);
	strshare_unref(state.email_canonical);

	logcommand(si, CMDLOG_ADMIN, "LISTMAIL: \2%s\2 (\2%d\2 matches)", email, state.matches);
	if (state.matches == 0)
		command_success_nodata(si, _("No accounts matched e-mail address \2%s\2"), email);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 match for e-mail address \2%s\2"),
						    N_("\2%d\2 matches for e-mail address \2%s\2"), state.matches), state.matches, email);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/listmail", MODULE_UNLOAD_CAPABILITY_OK)
