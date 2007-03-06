/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SENDPASS function.
 *
 * $Id: sendpass.c 7895 2007-03-06 02:40:03Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/sendpass", FALSE, _modinit, _moddeinit,
	"$Id: sendpass.c 7895 2007-03-06 02:40:03Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_sendpass(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_sendpass = { "SENDPASS", N_("Email registration passwords."), PRIV_USER_SENDPASS, 1, ns_cmd_sendpass };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_sendpass, ns_cmdtree);
	help_addentry(ns_helptree, "SENDPASS", "help/nickserv/sendpass", NULL);
}

void _moddeinit()
{
	command_delete(&ns_sendpass, ns_cmdtree);
	help_delentry(ns_helptree, "SENDPASS");
}

static void ns_cmd_sendpass(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	char *name = parv[0];
	char *newpass = NULL;
	char *key;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SENDPASS");
		command_fail(si, fault_needmoreparams, _("Syntax: SENDPASS <nickname>"));
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), name);
		return;
	}

	if (is_soper(mu) && !has_priv(si, PRIV_ADMIN))
	{
		logcommand(si, CMDLOG_ADMIN, "failed SENDPASS %s (is SOPER)", name);
		command_fail(si, fault_badparams, _("\2%s\2 belongs to a services operator; you need %s privilege to send the password."), name, PRIV_ADMIN);
		return;
	}

	/* alternative, safer method? */
	if (command_find(si->service->cmdtree, "SETPASS"))
	{
		key = gen_pw(12);
		if (sendemail(si->su != NULL ? si->su : si->service->me, EMAIL_SETPASS, mu, key))
		{
			metadata_add(mu, METADATA_USER, "private:setpass:key", crypt_string(key, gen_salt()));
			logcommand(si, CMDLOG_ADMIN, "SENDPASS %s (change key)", name);
			snoop("SENDPASS: \2%s\2 by \2%s\2", mu->name, get_oper_name(si));
			command_success_nodata(si, _("The password change key for \2%s\2 has been sent to \2%s\2."), mu->name, mu->email);
		}
		else
			command_fail(si, fault_emailfail, _("Email send failed."));
		free(key);
		return;
	}

	/* this is not without controversy... :) */
	if (mu->flags & MU_CRYPTPASS)
	{
		command_success_nodata(si, _("The password for the nickname \2%s\2 is encrypted; a new password will be assigned and sent."), name);
		newpass = gen_pw(12);
		set_password(mu, newpass);
	}

	if (sendemail(si->su != NULL ? si->su : si->service->me, EMAIL_SENDPASS, mu, (newpass == NULL) ? mu->pass : newpass))
	{
		logcommand(si, CMDLOG_ADMIN, "SENDPASS %s", name);
		snoop("SENDPASS: \2%s\2 by \2%s\2", mu->name, get_oper_name(si));
		command_success_nodata(si, _("The password for \2%s\2 has been sent to \2%s\2."), mu->name, mu->email);
	}
	else
		command_fail(si, fault_emailfail, _("Email send failed."));

	if (newpass != NULL)
		free(newpass);

	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
