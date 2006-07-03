/*
 * Copyright (c) 2005 Alex Lambert
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Implements USERSERV RETURN.
 *
 * $Id: return.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/return", FALSE, _modinit, _moddeinit,
	"$Id: return.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_return(char *origin);

command_t us_return = { "RETURN", "Returns an account to its owner.",
			PRIV_USER_ADMIN, us_cmd_return };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_return, us_cmdtree);
	help_addentry(us_helptree, "RETURN", "help/userserv/return", NULL);
}

void _moddeinit()
{
	command_delete(&us_return, us_cmdtree);
	help_delentry(us_helptree, "RETURN");
}

static void us_cmd_return(char *origin)
{
	user_t *u = user_find_named(origin);
	char *target = strtok(NULL, " ");
	char *newmail = strtok(NULL, " ");
	char *newpass;
	char oldmail[EMAILLEN];
	myuser_t *mu;

	if (u == NULL)
		return;

	if (!target || !newmail)
	{
		notice(usersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "RETURN");
		notice(usersvs.nick, origin, "Usage: RETURN <account> <e-mail address>");
		return;
	}

	if (!(mu = myuser_find(target)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", target);
		return;
	}

	if (is_soper(mu))
	{
		logcommand(usersvs.me, u, CMDLOG_ADMIN, "failed RETURN %s to %s (is SOPER)", target, newmail);
		notice(usersvs.nick, origin, "\2%s\2 belongs to a services operator; it cannot be returned.", target);
		return;
	}

	if ((strlen(newmail) > 32) || !validemail(newmail))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not a valid e-mail address.", newmail);
		return;
	}

	newpass = gen_pw(12);
	strlcpy(oldmail, mu->email, EMAILLEN);
	strlcpy(mu->email, newmail, EMAILLEN);

	if (!sendemail(u, EMAIL_SENDPASS, mu, newpass))
	{
		strlcpy(mu->email, oldmail, EMAILLEN);
		notice(usersvs.nick, origin, "Sending email failed, account \2%s\2 remains with \2%s\2.",
				mu->name, mu->email);
		return;
	}

	set_password(mu, newpass);

	free(newpass);

	/* prevents users from "stealing it back" in the event of a takeover */
	metadata_delete(mu, METADATA_USER, "private:verify:emailchg:key");
	metadata_delete(mu, METADATA_USER, "private:verify:emailchg:newemail");
	metadata_delete(mu, METADATA_USER, "private:verify:emailchg:timestamp");

	wallops("%s returned the account \2%s\2 to \2%s\2", origin, target, newmail);
	logcommand(usersvs.me, u, CMDLOG_ADMIN, "RETURN %s to %s", target, newmail);
	notice(usersvs.nick, origin, "The e-mail address for \2%s\2 has been set to \2%s\2",
						target, newmail);
	notice(usersvs.nick, origin, "A random password has been set; it has been sent to \2%s\2.",
						newmail);
}
