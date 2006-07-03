/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SENDPASS function.
 *
 * $Id: sendpass.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/sendpass", FALSE, _modinit, _moddeinit,
	"$Id: sendpass.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_sendpass(char *origin);

command_t us_sendpass = { "SENDPASS", "Email registration passwords.",
				PRIV_USER_ADMIN, us_cmd_sendpass };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_sendpass, us_cmdtree);
	help_addentry(us_helptree, "SENDPASS", "help/userserv/sendpass", NULL);
}

void _moddeinit()
{
	command_delete(&us_sendpass, us_cmdtree);
	help_delentry(us_helptree, "SENDPASS");
}

static void us_cmd_sendpass(char *origin)
{
	user_t *u = user_find_named(origin);
	myuser_t *mu;
	char *name = strtok(NULL, " ");
	char *newpass = NULL;

	if (u == NULL)
		return;

	if (!name)
	{
		notice(usersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "SENDPASS");
		notice(usersvs.nick, origin, "Syntax: SENDPASS <account>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (is_soper(mu) && !has_priv(u, PRIV_ADMIN))
	{
		logcommand(usersvs.me, u, CMDLOG_ADMIN, "failed SENDPASS %s (is SOPER)", name);
		notice(usersvs.nick, origin, "\2%s\2 belongs to a services operator; you need %s privilege to send the password.", name, PRIV_ADMIN);
		return;
	}

	/* this is not without controversy... :) */
	if (mu->flags & MU_CRYPTPASS)
	{
		notice(usersvs.nick, origin, "The password for the account \2%s\2 is encrypted; "
						"a new password will be assigned and sent.", name);
		newpass = gen_pw(12);
		set_password(mu, newpass);
	}

	if (sendemail(u, EMAIL_SENDPASS, mu, (newpass == NULL) ? mu->pass : newpass))
	{
		logcommand(usersvs.me, u, CMDLOG_ADMIN, "SENDPASS %s", name);
		notice(usersvs.nick, origin, "The password for \2%s\2 has been sent to \2%s\2.", mu->name, mu->email);
	}
	else
		notice(usersvs.nick, origin, "Email send failed.");

	if (newpass != NULL)
		free(newpass);

	return;
}
