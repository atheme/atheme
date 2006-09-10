/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SENDPASS function.
 *
 * $Id: sendpass.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/sendpass", FALSE, _modinit, _moddeinit,
	"$Id: sendpass.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_sendpass(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_sendpass = { "SENDPASS", "Email registration passwords.", PRIV_USER_ADMIN, 1, ns_cmd_sendpass };

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

	if (!name)
	{
		notice(nicksvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "SENDPASS");
		notice(nicksvs.nick, si->su->nick, "Syntax: SENDPASS <nickname>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, si->su->nick, "\2%s\2 is not registered.", name);
		return;
	}

	if (is_soper(mu) && !has_priv(si->su, PRIV_ADMIN))
	{
		logcommand(nicksvs.me, si->su, CMDLOG_ADMIN, "failed SENDPASS %s (is SOPER)", name);
		notice(nicksvs.nick, si->su->nick, "\2%s\2 belongs to a services operator; you need %s privilege to send the password.", name, PRIV_ADMIN);
		return;
	}

	/* this is not without controversy... :) */
	if (mu->flags & MU_CRYPTPASS)
	{
		notice(nicksvs.nick, si->su->nick, "The password for the nickname \2%s\2 is encrypted; "
						"a new password will be assigned and sent.", name);
		newpass = gen_pw(12);
		set_password(mu, newpass);
	}

	if (sendemail(si->su, EMAIL_SENDPASS, mu, (newpass == NULL) ? mu->pass : newpass))
	{
		logcommand(nicksvs.me, si->su, CMDLOG_ADMIN, "SENDPASS %s", name);
		notice(nicksvs.nick, si->su->nick, "The password for \2%s\2 has been sent to \2%s\2.", mu->name, mu->email);
	}
	else
		notice(nicksvs.nick, si->su->nick, "Email send failed.");

	if (newpass != NULL)
		free(newpass);

	return;
}
