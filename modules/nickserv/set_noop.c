/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Prevents services from setting modes upon you automatically.
 *
 */

#include "atheme.h"
#include "uplink.h"

DECLARE_MODULE_V1
(
	"nickserv/set_noop", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_patricia_t **ns_set_cmdtree;

static void ns_cmd_set_noop(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_set_noop = { "NOOP", N_("Prevents services from setting modes upon you automatically."), AC_NONE, 1, ns_cmd_set_noop, { .path = "nickserv/set_noop" } };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	command_add(&ns_set_noop, *ns_set_cmdtree);
}

void _moddeinit(void)
{
	command_delete(&ns_set_noop, *ns_set_cmdtree);
}

/* SET NOOP [ON|OFF] */
static void ns_cmd_set_noop(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (params == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOOP");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NOOP & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "NOOP", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOOP:ON");

		si->smu->flags |= MU_NOOP;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "NOOP", entity(si->smu)->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOOP & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "NOOP", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOOP:OFF");

		si->smu->flags &= ~MU_NOOP;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NOOP", entity(si->smu)->name);

		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NOOP");
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
