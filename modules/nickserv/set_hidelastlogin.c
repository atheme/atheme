/*
 * Copyright (c) 2016 Austin Ellis <siniStar -at- IRC4Fun.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Hides (opts you OUT) of the automatic Last Login notice upon
 * successfully authenticating to your account.
 *
 */

#include "atheme.h"
#include "uplink.h"

DECLARE_MODULE_V1
(
	"nickserv/set_hidelastlogin", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	VENDOR_STRING
);

mowgli_patricia_t **ns_set_cmdtree;

static void ns_cmd_set_hidelastlogin(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_set_hidelastlogin = { "HIDELASTLOGIN", N_("Opts you out of Last Login notices upon identifying to your account."), AC_NONE, 1, ns_cmd_set_hidelastlogin, { .path = "nickserv/set_hidelastlogin" } };

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	command_add(&ns_set_hidelastlogin, *ns_set_cmdtree);

}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&ns_set_hidelastlogin, *ns_set_cmdtree);
}

/* SET HIDELASTLOGIN [ON|OFF] */
static void ns_cmd_set_hidelastlogin(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = parv[0];


	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HIDELASTLOGIN");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (metadata_find(si->smu, "private:showlast:optout"))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "HIDELASTLOGIN", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:HIDELASTLOGIN:ON");

		metadata_add(si->smu, "private:showlast:optout", "ON");

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "HIDELASTLOGIN", entity(si->smu)->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!metadata_find(si->smu, "private:showlast:optout"))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "HIDELASTLOGIN", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:HIDELASTLOGIN:OFF");

		metadata_delete(si->smu, "private:showlast:optout");

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "HIDELASTLOGIN", entity(si->smu)->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "HIDELASTLOGIN");
		return;
	}
}


