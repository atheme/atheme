/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2007 William Pitcock, et al.
 *
 * This file contains routines to handle the NickServ SET PRIVMSG command.
 */

#include <atheme.h>
#include "list_common.h"
#include "list.h"

static mowgli_patricia_t **ns_set_cmdtree = NULL;

static bool
uses_privmsg(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return ( mu->flags & MU_USE_PRIVMSG ) == MU_USE_PRIVMSG;
}

// SET PRIVMSG ON|OFF
static void
ns_cmd_set_privmsg(struct sourceinfo *si, int parc, char *parv[])
{
	char *params = parv[0];

	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET PRIVMSG");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_USE_PRIVMSG & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for \2%s\2."), "PRIVMSG", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:PRIVMSG:ON");

		si->smu->flags |= MU_USE_PRIVMSG;

		command_success_nodata(si, _("The \2%s\2 flag has been set for \2%s\2."), "PRIVMSG" ,entity(si->smu)->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_USE_PRIVMSG & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for \2%s\2."), "PRIVMSG", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:PRIVMSG:OFF");

		si->smu->flags &= ~MU_USE_PRIVMSG;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for \2%s\2."), "PRIVMSG", entity(si->smu)->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET PRIVMSG");
		return;
	}
}

static struct command ns_set_privmsg = {
	.name           = "PRIVMSG",
	.desc           = N_("Uses private messages instead of notices if enabled."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_privmsg,
	.help           = { .path = "nickserv/set_privmsg" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	use_nslist_main_symbols(m);

	use_privmsg++;

	command_add(&ns_set_privmsg, *ns_set_cmdtree);

	static struct list_param use_privmsg;
	use_privmsg.opttype = OPT_BOOL;
	use_privmsg.is_match = uses_privmsg;

	list_register("use-privmsg", &use_privmsg);
	list_register("use_privmsg", &use_privmsg);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_privmsg, *ns_set_cmdtree);

	use_privmsg--;

	list_unregister("use-privmsg");
	list_unregister("use_privmsg");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_privmsg", MODULE_UNLOAD_CAPABILITY_OK)
