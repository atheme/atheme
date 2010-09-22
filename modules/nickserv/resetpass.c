/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for nickserv RESETPASS
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/resetpass", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_resetpass(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_resetpass = { "RESETPASS", N_("Resets an account password."), PRIV_USER_ADMIN, 1, ns_cmd_resetpass, { .path = "nickserv/resetpass" } };

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_resetpass);
}

void _moddeinit()
{
	service_named_unbind_command("nickserv", &ns_resetpass);
}

static void ns_cmd_resetpass(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	metadata_t *md;
	char *name = parv[0];
	char *newpass;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "RESETPASS");
		command_fail(si, fault_needmoreparams, _("Syntax: RESETPASS <account>"));
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), name);
		return;
	}

	if (is_soper(mu) && !has_priv(si, PRIV_ADMIN))
	{
		logcommand(si, CMDLOG_ADMIN, "failed RESETPASS \2%s\2 (is SOPER)", name);
		command_fail(si, fault_badparams, _("\2%s\2 belongs to a services operator; you need %s privilege to reset the password."), name, PRIV_ADMIN);
		return;
	}

	if ((md = metadata_find(mu, "private:mark:setter")) && has_priv(si, PRIV_MARK))
	{
		logcommand(si, CMDLOG_ADMIN, "RESETPASS: \2%s\2 (overriding mark by \2%s\2)", name, md->value);
		command_success_nodata(si, _("Overriding MARK placed by %s on the account %s."), md->value, name);
		newpass = gen_pw(12);
		command_success_nodata(si, _("The password for the account %s has been changed to %s."), name, newpass);
		set_password(mu, newpass);
		free(newpass);
		wallops("%s reset the password for the \2MARKED\2 account %s.", get_oper_name(si), name);
		return;
	}

	if ((md = metadata_find(mu, "private:mark:setter")))
	{
		logcommand(si, CMDLOG_ADMIN, "failed RESETPASS \2%s\2 (marked by \2%s\2)", name, md->value);
		command_fail(si, fault_badparams, _("This operation cannot be performed on %s, because the account has been marked by %s."), name, md->value);
		return;
	}

	newpass = gen_pw(12);
	command_success_nodata(si, _("The password for the account %s has been changed to %s."), name, newpass);
	set_password(mu, newpass);
	free(newpass);
	metadata_delete(mu, "private:setpass:key");

	wallops("%s reset the password for the account %s", get_oper_name(si), name);
	logcommand(si, CMDLOG_ADMIN, "RESETPASS: \2%s\2", name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
