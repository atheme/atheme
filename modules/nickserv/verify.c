/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ VERIFY function.
 *
 * $Id: verify.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/verify", FALSE, _modinit, _moddeinit,
	"$Id: verify.c 6337 2006-09-10 15:54:41Z pippijn $",
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
		notice(nicksvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "VERIFY");
		notice(nicksvs.nick, si->su->nick, "Syntax: VERIFY <operation> <nickname> <key>");
		return;
	}

	if (!(mu = myuser_find(nick)))
	{
		notice(nicksvs.nick, si->su->nick, "\2%s\2 is not registered.", nick);
		return;
	}

	/* forcing users to log in before we verify
	 * prevents some information leaks
	 */
	if (!(si->su->myuser == mu))
	{
		notice(nicksvs.nick, si->su->nick, "Please log in before attempting to verify your registration.");
		return;
	}

	if (!strcasecmp(op, "REGISTER"))
	{
		if (!(mu->flags & MU_WAITAUTH) || !(md = metadata_find(mu, METADATA_USER, "private:verify:register:key")))
		{
			notice(nicksvs.nick, si->su->nick, "\2%s\2 is not awaiting authorization.", nick);
			return;
		}

		if (!strcasecmp(key, md->value))
		{
			mu->flags &= ~MU_WAITAUTH;

			snoop("REGISTER:VS: \2%s\2 by \2%s\2", mu->email, si->su->nick);
			logcommand(nicksvs.me, si->su, CMDLOG_SET, "VERIFY REGISTER (email: %s)", mu->email);

			metadata_delete(mu, METADATA_USER, "private:verify:register:key");
			metadata_delete(mu, METADATA_USER, "private:verify:register:timestamp");

			notice(nicksvs.nick, si->su->nick, "\2%s\2 has now been verified.", mu->name);
			notice(nicksvs.nick, si->su->nick, "Thank you for verifying your e-mail address! You have taken steps in "
				"ensuring that your registrations are not exploited.");
			ircd_on_login(si->su->nick, mu->name, NULL);

			return;
		}

		snoop("REGISTER:VF: \2%s\2 by \2%s\2", mu->email, si->su->nick);
		logcommand(nicksvs.me, si->su, CMDLOG_SET, "failed VERIFY REGISTER (invalid key)");
		notice(nicksvs.nick, si->su->nick, "Verification failed. Invalid key for \2%s\2.", mu->name);

		return;
	}
	else if (!strcasecmp(op, "EMAILCHG"))
	{
		if (!(md = metadata_find(mu, METADATA_USER, "private:verify:emailchg:key")))
		{
			notice(nicksvs.nick, si->su->nick, "\2%s\2 is not awaiting authorization.", nick);
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

			notice(nicksvs.nick, si->su->nick, "\2%s\2 has now been verified.", mu->email);

			return;
		}

		snoop("REGISTER:VF: \2%s\2 by \2%s\2", mu->email, si->su->nick);
		logcommand(nicksvs.me, si->su, CMDLOG_SET, "failed VERIFY EMAILCHG (invalid key)");
		notice(nicksvs.nick, si->su->nick, "Verification failed. Invalid key for \2%s\2.", mu->name);

		return;
	}
	else
	{
		notice(nicksvs.nick, si->su->nick, "Invalid operation specified for \2VERIFY\2.");
		notice(nicksvs.nick, si->su->nick, "Please double-check your verification e-mail.");
		return;
	}
}
