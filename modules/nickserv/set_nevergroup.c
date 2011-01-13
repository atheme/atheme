/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Prevents you from being added to group access lists.
 *
 */

#include "atheme.h"
#include "uplink.h"

DECLARE_MODULE_V1
(
	"nickserv/set_nevergroup", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_patricia_t **ns_set_cmdtree;

static void ns_cmd_set_nevergroup(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_set_nevergroup = { "NEVERGROUP", N_("Prevents you from being added to group access lists."), AC_NONE, 1, ns_cmd_set_nevergroup, { .path = "nickserv/set_nevergroup" } };

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	command_add(&ns_set_nevergroup, *ns_set_cmdtree);
}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&ns_set_nevergroup, *ns_set_cmdtree);
}

/* SET NEVERGROUP <ON|OFF> */
static void ns_cmd_set_nevergroup(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = parv[0];

	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NEVERGROUP");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NEVERGROUP & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "NEVERGROUP", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NEVERGROUP:ON");

		si->smu->flags |= MU_NEVERGROUP;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "NEVERGROUP", entity(si->smu)->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NEVERGROUP & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "NEVERGROUP", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NEVERGROUP:OFF");

		si->smu->flags &= ~MU_NEVERGROUP;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NEVERGROUP", entity(si->smu)->name);

		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NEVERGROUP");
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
