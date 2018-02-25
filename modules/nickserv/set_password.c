/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Changes the password associated with your account.
 */

#include "atheme.h"
#include "uplink.h"

mowgli_patricia_t **ns_set_cmdtree;

static void ns_cmd_set_password(struct sourceinfo *si, int parc, char *parv[]);

static struct command ns_set_password = { "PASSWORD", N_("Changes the password associated with your account."), AC_NONE, 1, ns_cmd_set_password, { .path = "nickserv/set_password" } };

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	command_add(&ns_set_password, *ns_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_password, *ns_set_cmdtree);
}

/* SET PASSWORD <password> */
static void
ns_cmd_set_password(struct sourceinfo *si, int parc, char *parv[])
{
	char *password = parv[0];

	if (auth_module_loaded)
	{
		command_fail(si, fault_noprivs, _("You must change the password in the external system."));
		return;
	}

	if (!password)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "PASSWORD");
		return;
	}

	if (strlen(password) > PASSLEN)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "PASSWORD");
		command_fail(si, fault_badparams, _("Registration passwords may not be longer than \2%u\2 characters."), PASSLEN);
		return;
	}

	if (!strcasecmp(password, entity(si->smu)->name))
	{
		command_fail(si, fault_badparams, _("You cannot use your nickname as a password."));
		command_fail(si, fault_badparams, _("Syntax: SET PASSWORD <new password>"));
		return;
	}

	logcommand(si, CMDLOG_SET, "SET:PASSWORD");

	set_password(si->smu, password);

	command_success_nodata(si, _("The password for \2%s\2 has been changed to \2%s\2."), entity(si->smu)->name, password);

	return;
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_password", MODULE_UNLOAD_CAPABILITY_OK)
