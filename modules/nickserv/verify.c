/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ VERIFY function.
 *
 * $Id: verify.c 6519 2006-09-27 22:44:37Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/verify", FALSE, _modinit, _moddeinit,
	"$Id: verify.c 6519 2006-09-27 22:44:37Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_verify(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_verify = { "VERIFY", "Verifies a nickname registration.", AC_NONE, 3, ns_cmd_verify };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_verify, ns_cmdtree);
	help_addentry(ns_helptree, "VERIFY", "help/nickserv/verify", NULL);
}

void _moddeinit()
{
	command_delete(&ns_verify, ns_cmdtree);
	help_delentry(ns_helptree, "VERIFY");
}

static void ns_cmd_verify(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	metadata_t *md;
	char *op = parv[0];
	char *nick = parv[1];
	char *key = parv[2];

	if (!op || !nick || !key)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "VERIFY");
		command_fail(si, fault_needmoreparams, "Syntax: VERIFY <operation> <nickname> <key>");
		return;
	}

	if (!(mu = myuser_find(nick)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", nick);
		return;
	}

	/* forcing users to log in before we verify
	 * prevents some information leaks
	 */
	if (!(si->smu == mu))
	{
		command_fail(si, fault_badparams, "Please log in before attempting to verify your registration.");
		return;
	}

	if (!strcasecmp(op, "REGISTER"))
	{
		if (!(mu->flags & MU_WAITAUTH) || !(md = metadata_find(mu, METADATA_USER, "private:verify:register:key")))
		{
			command_fail(si, fault_badparams, "\2%s\2 is not awaiting authorization.", nick);
			return;
		}

		if (!strcasecmp(key, md->value))
		{
			mu->flags &= ~MU_WAITAUTH;

			snoop("REGISTER:VS: \2%s\2 by \2%s\2", mu->email, si->su->nick);
			logcommand(nicksvs.me, si->su, CMDLOG_SET, "VERIFY REGISTER (email: %s)", mu->email);

			metadata_delete(mu, METADATA_USER, "private:verify:register:key");
			metadata_delete(mu, METADATA_USER, "private:verify:register:timestamp");

			command_success_nodata(si, "\2%s\2 has now been verified.", mu->name);
			command_success_nodata(si, "Thank you for verifying your e-mail address! You have taken steps in "
				"ensuring that your registrations are not exploited.");
			ircd_on_login(si->su->nick, mu->name, NULL);

			return;
		}

		snoop("REGISTER:VF: \2%s\2 by \2%s\2", mu->email, si->su->nick);
		logcommand(nicksvs.me, si->su, CMDLOG_SET, "failed VERIFY REGISTER (invalid key)");
		command_fail(si, fault_badparams, "Verification failed. Invalid key for \2%s\2.", 
			mu->name);

		return;
	}
	else if (!strcasecmp(op, "EMAILCHG"))
	{
		if (!(md = metadata_find(mu, METADATA_USER, "private:verify:emailchg:key")))
		{
			command_fail(si, fault_badparams, "\2%s\2 is not awaiting authorization.", nick);
			return;
		}

		if (!strcasecmp(key, md->value))
		{
			md = metadata_find(mu, METADATA_USER, "private:verify:emailchg:newemail");

			strlcpy(mu->email, md->value, EMAILLEN);

			snoop("SET:EMAIL:VS: \2%s\2 by \2%s\2", mu->email, si->su->nick);
			logcommand(nicksvs.me, si->su, CMDLOG_SET, "VERIFY EMAILCHG (email: %s)", mu->email);

			metadata_delete(mu, METADATA_USER, "private:verify:emailchg:key");
			metadata_delete(mu, METADATA_USER, "private:verify:emailchg:newemail");
			metadata_delete(mu, METADATA_USER, "private:verify:emailchg:timestamp");

			command_success_nodata(si, "\2%s\2 has now been verified.", mu->email);

			return;
		}

		snoop("REGISTER:VF: \2%s\2 by \2%s\2", mu->email, si->su->nick);
		logcommand(nicksvs.me, si->su, CMDLOG_SET, "failed VERIFY EMAILCHG (invalid key)");
		command_fail(si, fault_badparams, "Verification failed. Invalid key for \2%s\2.", 
			mu->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, "Invalid operation specified for \2VERIFY\2.");
		command_fail(si, fault_badparams, "Please double-check your verification e-mail.");
		return;
	}
}
