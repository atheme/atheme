/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 * Copyright (C) 2022 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

#define BADPASSWDMSG_MDNAME "private:badpasswdmsg"

static mowgli_patricia_t **ns_set_cmdtree = NULL;

static void
ns_cmd_set_badpasswdmsg(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET BADPASSWDMSG");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET BADPASSWDMSG <ON|OFF|DEFAULT>"));
		return;
	}

	const struct metadata *const md = metadata_find(si->smu, BADPASSWDMSG_MDNAME);

	if (strcasecmp(parv[0], "ON") == 0)
	{
		if (md && strcmp(md->value, "1") == 0)
		{
			(void) command_fail(si, fault_nochange, _("The \2%s\2 flag is already \2ON\2 for account \2%s\2."),
			                                          "BADPASSWDMSG", entity(si->smu)->name);
			return;
		}

		(void) metadata_add(si->smu, BADPASSWDMSG_MDNAME, "1");

		(void) logcommand(si, CMDLOG_SET, "SET:BADPASSWDMSG:ON");
		(void) command_success_nodata(si, _("The \2%s\2 flag has been set \2ON\2 for account \2%s\2."),
		                                    "BADPASSWDMSG", entity(si->smu)->name);
	}
	else if (strcasecmp(parv[0], "OFF") == 0)
	{
		if (md && strcmp(md->value, "0") == 0)
		{
			(void) command_fail(si, fault_nochange, _("The \2%s\2 flag is already \2OFF\2 for account \2%s\2."),
			                                          "BADPASSWDMSG", entity(si->smu)->name);
			return;
		}

		(void) metadata_add(si->smu, BADPASSWDMSG_MDNAME, "0");

		(void) logcommand(si, CMDLOG_SET, "SET:BADPASSWDMSG:OFF");
		(void) command_success_nodata(si, _("The \2%s\2 flag has been set \2OFF\2 for account \2%s\2."),
		                                    "BADPASSWDMSG", entity(si->smu)->name);
	}
	else if (strcasecmp(parv[0], "DEFAULT") == 0)
	{
		if (! md)
		{
			(void) command_fail(si, fault_nochange, _("The \2%s\2 flag is already \2DEFAULT\2 for account \2%s\2."),
			                                          "BADPASSWDMSG", entity(si->smu)->name);
			return;
		}

		(void) metadata_delete(si->smu, BADPASSWDMSG_MDNAME);

		(void) logcommand(si, CMDLOG_SET, "SET:BADPASSWDMSG:DEFAULT");
		(void) command_success_nodata(si, _("The \2%s\2 flag has been set \2DEFAULT\2 for account \2%s\2."),
		                                    "BADPASSWDMSG", entity(si->smu)->name);
	}
	else
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET BADPASSWDMSG");
}

static struct command ns_set_badpasswdmsg = {
	.name           = "BADPASSWDMSG",
	.desc           = N_("Notifies you of failed password-based login attempts."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_badpasswdmsg,
	.help           = { .path = "nickserv/set_badpasswdmsg" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	(void) command_add(&ns_set_badpasswdmsg, *ns_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) command_delete(&ns_set_badpasswdmsg, *ns_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_badpasswdmsg", MODULE_UNLOAD_CAPABILITY_OK)
