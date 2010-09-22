/*
 * Copyright (c) 2005-2008 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LISTOWNMAIL function.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/listownmail", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_listownmail(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_listownmail = { "LISTOWNMAIL", N_("Lists accounts registered to your e-mail address."), AC_NONE, 1, ns_cmd_listownmail, { .path = "nickserv/listownmail" } };

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_listownmail);
}

void _moddeinit()
{
	service_named_unbind_command("nickserv", &ns_listownmail);
}

struct listmail_state
{
	sourceinfo_t *origin;
	char *pattern;
	int matches;
};

static int listmail_foreach_cb(myentity_t *mt, void *privdata)
{
	struct listmail_state *state = (struct listmail_state *) privdata;
	myuser_t *mu = user(mt);

	if (!strcasecmp(state->pattern, mu->email))
	{
		/* in the future we could add a LIMIT parameter */
		if (state->matches == 0)
			command_success_nodata(state->origin, "Accounts matching e-mail address \2%s\2:", state->pattern);

		command_success_nodata(state->origin, "- %s (%s)", entity(mu)->name, mu->email);
		state->matches++;
	}

	return 0;
}

static void ns_cmd_listownmail(sourceinfo_t *si, int parc, char *parv[])
{
	struct listmail_state state;

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_noprivs, _("You have to verify your email address before you can perform this operation."));
		return;
	}

	command_add_flood(si, FLOOD_HEAVY);

	state.matches = 0;
	state.pattern = si->smu->email;
	state.origin = si;
	myentity_foreach_t(ENT_USER, listmail_foreach_cb, &state);

	logcommand(si, CMDLOG_GET, "LISTOWNMAIL: \2%s\2 (\2%d\2 matches)", si->smu->email, state.matches);
	command_success_nodata(si, ngettext(N_("\2%d\2 match for e-mail address \2%s\2"),
					    N_("\2%d\2 matches for e-mail address \2%s\2"), state.matches), state.matches, si->smu->email);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
