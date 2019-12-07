/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2011 William Pitcock, et al.
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains routines to handle the OperServ SET command.
 */

#include <atheme.h>

static mowgli_patricia_t **os_set_cmdtree = NULL;

static void
os_cmd_set_spam_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET SPAM");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET SPAM <TRUE|FALSE>"));
		return;
	}

	const char *const param = parv[0];

	if (!strcasecmp("TRUE", param) || !strcasecmp("ON", param))
	{
		if (nicksvs.spam)
		{
			(void) command_fail(si, fault_nochange, _("The SPAM directive is already set."));
			return;
		}

		nicksvs.spam = true;

		(void) command_success_nodata(si, _("The SPAM directive has been successfully set."));
		(void) logcommand(si, CMDLOG_ADMIN, "SET:SPAM:TRUE");
	}
	else if (!strcasecmp("FALSE", param) || !strcasecmp("OFF", param))
	{
		if (!nicksvs.spam)
		{
			(void) command_fail(si, fault_nochange, _("The SPAM directive is already unset."));
			return;
		}

		nicksvs.spam = false;

		(void) command_success_nodata(si, _("The SPAM directive has been successfully unset."));
		(void) logcommand(si, CMDLOG_ADMIN, "SET:SPAM:FALSE");
	}
	else
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET SPAM");
		(void) command_fail(si, fault_badparams, _("Syntax: SET SPAM <TRUE|FALSE>"));
	}
}

static struct command os_cmd_set_spam = {
	.name           = "SPAM",
	.desc           = N_("Changes whether service spams unregistered users on connect."),
	.access         = PRIV_ADMIN,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_spam_func,
	.help           = { .path = "oservice/set_spam" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, os_set_cmdtree, "operserv/set_core", "os_set_cmdtree")

	(void) command_add(&os_cmd_set_spam, *os_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) command_delete(&os_cmd_set_spam, *os_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("operserv/set_spam", MODULE_UNLOAD_CAPABILITY_OK)
