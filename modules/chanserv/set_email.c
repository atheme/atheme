/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2006-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the CService SET EMAIL command.
 */

#include <atheme.h>

static mowgli_patricia_t **cs_set_cmdtree = NULL;

static void
cs_cmd_set_email(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	char *mail = parv[1];

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, parv[0]);
		return;
	}

	if (!mail || !strcasecmp(mail, "NONE") || !strcasecmp(mail, "OFF"))
	{
		if (metadata_find(mc, "email"))
		{
			metadata_delete(mc, "email");
			command_success_nodata(si, _("The e-mail address for channel \2%s\2 was deleted."), mc->name);
			logcommand(si, CMDLOG_SET, "SET:EMAIL:NONE: \2%s\2", mc->name);
			return;
		}

		command_fail(si, fault_nochange, _("The e-mail address for channel \2%s\2 was not set."), mc->name);
		return;
	}

	if (!validemail(mail))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid e-mail address."), mail);
		return;
	}

	// we'll overwrite any existing metadata
	metadata_add(mc, "email", mail);

	logcommand(si, CMDLOG_SET, "SET:EMAIL: \2%s\2 \2%s\2", mc->name, mail);
	verbose(mc, "\2%s\2 set the e-mail address for the channel to \2%s\2", get_source_name(si), mail);
	command_success_nodata(si, _("The e-mail address for channel \2%s\2 has been set to \2%s\2."), parv[0], mail);
}

static struct command cs_set_email = {
	.name           = "EMAIL",
	.desc           = N_("Sets the channel e-mail address."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_set_email,
	.help           = { .path = "cservice/set_email" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree")

	command_add(&cs_set_email, *cs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&cs_set_email, *cs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/set_email", MODULE_UNLOAD_CAPABILITY_OK)
