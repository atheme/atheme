/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (c) 2007 Jilles Tjoelker
 * Copyright (c) 2015 Max Teufel <max@teufelsnetz.com>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows you to opt-out of password-based authentication methods.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "list_common.h"
#include "list.h"

DECLARE_MODULE_V1
(
	"nickserv/set_nopassword", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	VENDOR_STRING
);

mowgli_patricia_t **ns_set_cmdtree;

static void ns_cmd_set_nopassword(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_set_nopassword = { "NOPASSWORD", N_("Allows you to disable any password-based authentication methods except for XMLRPC/JSONRPC."), AC_NONE, 1, ns_cmd_set_nopassword, { .path = "nickserv/set_nopassword" } };

static bool has_nopassword(const mynick_t *mn, const void *arg)
{
	myuser_t *mu = mn->owner;

	return ( mu->flags & MU_NOPASSWORD ) == MU_NOPASSWORD;
}

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	command_add(&ns_set_nopassword, *ns_set_cmdtree);

	use_nslist_main_symbols(m);

	static list_param_t nopassword;
	nopassword.opttype = OPT_BOOL;
	nopassword.is_match = has_nopassword;

	list_register("nopassword", &nopassword);
}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&ns_set_nopassword, *ns_set_cmdtree);

	list_unregister("nopassword");
}

/* SET NOPASSWORD [ON|OFF] */
static void ns_cmd_set_nopassword(sourceinfo_t *si, int parc, char *parv[])
{
	char *setting = parv[0];

	if (!setting)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOPASSWORD");
		return;
	}

	if (!strcasecmp("ON", setting))
	{
		if (MU_NOPASSWORD & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "NOPASSWORD", entity(si->smu)->name);
			return;
		}

		if (si->smu->cert_fingerprints.head == NULL && metadata_find(si->smu, "private:pubkey") == NULL && metadata_find(si->smu, "pubkey") == NULL && metadata_find(si->smu, "ecdsa-nist521p-pubkey") == NULL)
		{
			command_fail(si, fault_nochange, _("You are trying to enable NoPassword without any possibilty to identify without a password."));
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOPASSWORD:ON");

		si->smu->flags |= MU_NOPASSWORD;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "NOPASSWORD" ,entity(si->smu)->name);

		return;
	}
	else if (!strcasecmp("OFF", setting))
	{
		if (!(MU_NOPASSWORD & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "NOPASSWORD", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOPASSWORD:OFF");

		si->smu->flags &= ~MU_NOPASSWORD;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NOPASSWORD", entity(si->smu)->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NOPASSWORD");
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
