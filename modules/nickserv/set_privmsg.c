/*
 * Copyright (c) 2006-2007 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the NickServ SET PRIVMSG command.
 */

#include "atheme.h"
#include "list_common.h"
#include "list.h"

DECLARE_MODULE_V1
(
	"nickserv/set_privmsg", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	VENDOR_STRING
);

mowgli_patricia_t **ns_set_cmdtree;

/* SET PRIVMSG ON|OFF */
static void ns_cmd_set_privmsg(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = parv[0];

	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "PRIVMSG");
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
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "PRIVMSG");
		return;
	}
}

command_t ns_set_privmsg = { "PRIVMSG", N_("Uses private messages instead of notices if enabled."), AC_NONE, 1, ns_cmd_set_privmsg, { .path = "nickserv/set_privmsg" } };

static bool uses_privmsg(const mynick_t *mn, const void *arg)
{
	myuser_t *mu = mn->owner;

	return ( mu->flags & MU_USE_PRIVMSG ) == MU_USE_PRIVMSG;
}

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");
	command_add(&ns_set_privmsg, *ns_set_cmdtree);

	use_privmsg++;

	use_nslist_main_symbols(m);

	static list_param_t use_privmsg;
	use_privmsg.opttype = OPT_BOOL;
	use_privmsg.is_match = uses_privmsg;

	list_register("use-privmsg", &use_privmsg);
	list_register("use_privmsg", &use_privmsg);
}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&ns_set_privmsg, *ns_set_cmdtree);

	use_privmsg--;

	list_unregister("use-privmsg");
	list_unregister("use_privmsg");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
