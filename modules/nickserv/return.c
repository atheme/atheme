/*
 * Copyright (c) 2005 Alex Lambert
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Implements nickserv RETURN.
 *
 * $Id: return.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/return", FALSE, _modinit, _moddeinit,
	"$Id: return.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_return(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_return = { "RETURN", "Returns a nickname to its owner.", PRIV_USER_ADMIN, 2, ns_cmd_return };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_return, ns_cmdtree);
	help_addentry(ns_helptree, "RETURN", "help/nickserv/return", NULL);
}

void _moddeinit()
{
	command_delete(&ns_return, ns_cmdtree);
	help_delentry(ns_helptree, "RETURN");
}

static void ns_cmd_return(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *newmail = parv[1];
	char *newpass;
	char oldmail[EMAILLEN];
	myuser_t *mu;

	if (!target || !newmail)
	{
		notice(nicksvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "RETURN");
		notice(nicksvs.nick, si->su->nick, "Usage: RETURN <nickname> <e-mail address>");
		return;
	}

	if (!(mu = myuser_find(target)))
	{
		notice(nicksvs.nick, si->su->nick, "\2%s\2 is not registered.", target);
		return;
	}

	if (is_soper(mu))
	{
		logcommand(nicksvs.me, si->su, CMDLOG_ADMIN, "failed RETURN %s to %s (is SOPER)", target, newmail);
		notice(nicksvs.nick, si->su->nick, "\2%s\2 belongs to a services operator; it cannot be returned.", target);
		return;
	}

	if ((strlen(newmail) > 32) || !validemail(newmail))
	{
		notice(nicksvs.nick, si->su->nick, "\2%s\2 is not a valid e-mail address.", newmail);
		return;
	}

	newpass = gen_pw(12);
	strlcpy(oldmail, mu->email, EMAILLEN);
	strlcpy(mu->email, newmail, EMAILLEN);

	if (!sendemail(si->su, EMAIL_SENDPASS, mu, newpass))
	{
		strlcpy(mu->email, oldmail, EMAILLEN);
		notice(nicksvs.nick, si->su->nick, "Sending email failed, nickname \2%s\2 remains with \2%s\2.",
				mu->name, mu->email);
		return;
	}

	set_password(mu, newpass);

	free(newpass);

	/* prevents users from "stealing it back" in the event of a takeover */
	metadata_delete(mu, METADATA_USER, "private:verify:emailchg:key");
	metadata_delete(mu, METADATA_USER, "private:verify:emailchg:newemail");
	metadata_delete(mu, METADATA_USER, "private:verify:emailchg:timestamp");

	wallops("%s returned the nickname \2%s\2 to \2%s\2", si->su->nick, target, newmail);
	logcommand(nicksvs.me, si->su, CMDLOG_ADMIN, "RETURN %s to %s", target, newmail);
	notice(nicksvs.nick, si->su->nick, "The e-mail address for \2%s\2 has been set to \2%s\2",
						target, newmail);
	notice(nicksvs.nick, si->su->nick, "A random password has been set; it has been sent to \2%s\2.",
						newmail);
}
