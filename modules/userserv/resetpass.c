/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for UserServ RESETPASS
 *
 * $Id: resetpass.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/resetpass", FALSE, _modinit, _moddeinit,
	"$Id: resetpass.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_resetpass(sourceinfo_t *si, int parc, char *parv[]);

command_t us_resetpass = { "RESETPASS", "Resets an account password.", PRIV_USER_ADMIN, 1, us_cmd_resetpass };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_resetpass, us_cmdtree);
	help_addentry(us_helptree, "RESETPASS", "help/userserv/resetpass", NULL);
}

void _moddeinit()
{
	command_delete(&us_resetpass, us_cmdtree);
	help_delentry(us_helptree, "RESETPASS");
}

static void us_cmd_resetpass(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	metadata_t *md;
	char *name = parv[0];
	char *newpass;

	if (!name)
	{
		notice(usersvs.nick, si->su->nick, STR_INVALID_PARAMS, "RESETPASS");
		notice(usersvs.nick, si->su->nick, "Syntax: RESETPASS <account>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(usersvs.nick, si->su->nick, "\2%s\2 is not registered.", name);
		return;
	}

	if (is_soper(mu) && !has_priv(si->su, PRIV_ADMIN))
	{
		logcommand(usersvs.me, si->su, CMDLOG_ADMIN, "failed RESETPASS %s (is SOPER)", name);
		notice(usersvs.nick, si->su->nick, "\2%s\2 belongs to a services operator; you need %s privilege to reset the password.", name, PRIV_ADMIN);
		return;
	}

	if ((md = metadata_find(mu, METADATA_USER, "private:mark:setter")) && has_priv(si->su, PRIV_MARK))
	{
		logcommand(usersvs.me, si->su, CMDLOG_ADMIN, "RESETPASS %s (overriding mark by %s)", name, md->value);
		notice(usersvs.nick, si->su->nick, "Overriding MARK placed by %s on the account %s.", md->value, name);
		newpass = gen_pw(12);
		notice(usersvs.nick, si->su->nick, "The password for the account %s has been changed to %s.", name, newpass);
		set_password(mu, newpass);
		free(newpass);
		wallops("%s reset the password for the \2MARKED\2 account %s.", si->su->nick, name);
		return;
	}

	if ((md = metadata_find(mu, METADATA_USER, "private:mark:setter")))
	{
		logcommand(usersvs.me, si->su, CMDLOG_ADMIN, "failed RESETPASS %s (marked by %s)", name, md->value);
		notice(usersvs.nick, si->su->nick, "This operation cannot be performed on %s, because the account has been marked by %s.", name, md->value);
		return;
	}

	newpass = gen_pw(12);
	notice(usersvs.nick, si->su->nick, "The password for the account %s has been changed to %s.", name, newpass);
	set_password(mu, newpass);
	free(newpass);

	wallops("%s reset the password for the account %s", si->su->nick, name);
	snoop("RESETPASS: \2%s\2 reset the password for \2%s\2", si->su->nick, name);
	logcommand(usersvs.me, si->su, CMDLOG_ADMIN, "RESETPASS %s", name);

}
