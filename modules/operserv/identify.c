/*
 * Copyright (c) 2006 Stephen Bennett
 * Copyright (c) 2008 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService IDENTIFY command.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/identify", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_identify(sourceinfo_t *si, int parc, char *parv[]);

command_t os_identify = { "IDENTIFY", N_("Authenticate for services operator privileges."), AC_NONE, 1, os_cmd_identify, { .path = "oservice/identify" } };
command_t os_id = { "ID", N_("Alias for IDENTIFY"), AC_NONE, 1, os_cmd_identify, { .path = "oservice/identify" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_identify);
        service_named_bind_command("operserv", &os_id);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_identify);
	service_named_unbind_command("operserv", &os_id);
}

static bool verify_operserv_password(soper_t *so, char *password)
{
	if (so == NULL || password == NULL)
		return false;

	if (crypto_module_loaded)
		return crypt_verify_password(password, so->password);
	else
		return !strcmp(password, so->password);
}

static void os_cmd_identify(sourceinfo_t *si, int parc, char *parv[])
{
	/* XXX use this with authcookie also */
	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), "IDENTIFY");
		return;
	}

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IDENTIFY");
		command_fail(si, fault_needmoreparams, _("Syntax: IDENTIFY <password>"));
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!is_soper(si->smu))
	{
		command_fail(si, fault_noprivs, _("You do not have a services operator account."));
		return;
	}

	if (si->smu->soper->password == NULL)
	{
		command_fail(si, fault_nochange, _("Your services operator account does not have a password set."));
		return;
	}

	if (si->su->flags & UF_SOPER_PASS)
	{
		command_fail(si, fault_nochange, _("You are already identified to %s."), si->service->nick);
		return;
	}

	if (!verify_operserv_password(si->smu->soper, parv[0]))
	{
		command_fail(si, fault_authfail, _("Invalid password."));
		logcommand(si, CMDLOG_ADMIN, "failed IDENTIFY (bad password)");
		return;
	}

	si->su->flags |= UF_SOPER_PASS;
	command_success_nodata(si, _("You are now identified to %s."),
			si->service->nick);
	logcommand(si, CMDLOG_ADMIN, "IDENTIFY");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
