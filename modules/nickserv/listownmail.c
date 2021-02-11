/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2008 William Pitcock, et al.
 *
 * This file contains code for the NickServ LISTOWNMAIL function.
 */

#include <atheme.h>

static bool listownmail_canon = false;

static void
ns_cmd_listownmail(struct sourceinfo *si, int parc, char *parv[])
{
	struct myentity *mt;
	struct myentity_iteration_state state;
	unsigned int matches = 0;

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, STR_EMAIL_NOT_VERIFIED);
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
		struct myuser *mu = user(mt);

		continue_if_fail(mu != NULL);

		if ((listownmail_canon && !strcasecmp(si->smu->email_canonical, mu->email_canonical))
		 || (!listownmail_canon && !strcasecmp(si->smu->email, mu->email)))
		{
			// in the future we could add a LIMIT parameter
			if (matches == 0)
				command_success_nodata(si, _("Accounts matching e-mail address \2%s\2:"), si->smu->email);

			command_success_nodata(si, "- %s (%s)", entity(mu)->name, mu->email);
			matches++;
		}
	}

	logcommand(si, CMDLOG_GET, "LISTOWNMAIL: \2%s\2 (\2%u\2 matches)", si->smu->email, matches);
	command_success_nodata(si, ngettext(N_("\2%u\2 match for e-mail address \2%s\2"),
					    N_("\2%u\2 matches for e-mail address \2%s\2"), matches), matches, si->smu->email);
}

static struct command ns_listownmail = {
	.name           = "LISTOWNMAIL",
	.desc           = N_("Lists accounts registered to your e-mail address."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &ns_cmd_listownmail,
	.help           = { .path = "nickserv/listownmail" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	service_named_bind_command("nickserv", &ns_listownmail);
	add_bool_conf_item("LISTOWNMAIL_CANON", &nicksvs.me->conf_table, 0, &listownmail_canon, true);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_listownmail);
	del_conf_item("LISTOWNMAIL_CANON", &nicksvs.me->conf_table);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/listownmail", MODULE_UNLOAD_CAPABILITY_OK)
