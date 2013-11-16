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

command_t ns_listownmail = { "LISTOWNMAIL", N_("Lists accounts registered to your e-mail address."), AC_AUTHENTICATED, 1, ns_cmd_listownmail, { .path = "nickserv/listownmail" } };

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_listownmail);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("nickserv", &ns_listownmail);
}

static void ns_cmd_listownmail(sourceinfo_t *si, int parc, char *parv[])
{
	myentity_t *mt;
	myentity_iteration_state_t state;
	unsigned int matches = 0;

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_noprivs, _("You have to verify your email address before you can perform this operation."));
		return;
	}

	/* Normally this couldn't fail due to the verification check above,
	 * except when accounts have been imported from another services
	 * database that didn't require them, in which case lots of unrelated
	 * accounts may have 'noemail' or similar.
	 */
	if (!validemail(si->smu->email))
	{
		command_fail(si, fault_noprivs, _("You must have a valid email address to perform this operation."));
		return;
	}

	command_add_flood(si, FLOOD_HEAVY);

	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		myuser_t *mu = user(mt);

		continue_if_fail(mu != NULL);

		if (!strcasecmp(si->smu->email, mu->email))
		{
			/* in the future we could add a LIMIT parameter */
			if (matches == 0)
				command_success_nodata(si, "Accounts matching e-mail address \2%s\2:", si->smu->email);

			command_success_nodata(si, "- %s (%s)", entity(mu)->name, mu->email);
			matches++;
		}
	}

	logcommand(si, CMDLOG_GET, "LISTOWNMAIL: \2%s\2 (\2%d\2 matches)", si->smu->email, matches);
	command_success_nodata(si, ngettext(N_("\2%d\2 match for e-mail address \2%s\2"),
					    N_("\2%d\2 matches for e-mail address \2%s\2"), matches), matches, si->smu->email);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
