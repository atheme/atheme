/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the NickServ LISTMAIL function.
 */

#include <atheme.h>

struct listmail_state
{
	struct sourceinfo *origin;
	char *pattern;
	stringref email_canonical;
	unsigned int matches;
};

static int
listmail_foreach_cb(struct myentity *mt, void *privdata)
{
	struct listmail_state *state = (struct listmail_state *) privdata;
	struct myuser *mu = user(mt);

	if (state->email_canonical == mu->email_canonical || !match(state->pattern, mu->email))
	{
		// in the future we could add a LIMIT parameter
		if (state->matches == 0)
			command_success_nodata(state->origin, _("Accounts matching e-mail address \2%s\2:"), state->pattern);

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

	logcommand(si, CMDLOG_ADMIN, "LISTMAIL: \2%s\2 (\2%u\2 matches)", email, state.matches);
	if (state.matches == 0)
		command_success_nodata(si, _("No accounts matched e-mail address \2%s\2"), email);
	else
		command_success_nodata(si, ngettext(N_("\2%u\2 match for e-mail address \2%s\2"),
						    N_("\2%u\2 matches for e-mail address \2%s\2"), state.matches), state.matches, email);
}

static struct command ns_listmail = {
	.name           = "LISTMAIL",
	.desc           = N_("Lists accounts registered to an e-mail address."),
	.access         = PRIV_USER_AUSPEX,
	.maxparc        = 1,
	.cmd            = &ns_cmd_listmail,
	.help           = { .path = "nickserv/listmail" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	service_named_bind_command("nickserv", &ns_listmail);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_listmail);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/listmail", MODULE_UNLOAD_CAPABILITY_OK)
