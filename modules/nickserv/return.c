/*
 * Copyright (c) 2005 Alex Lambert
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Implements NICKSERV RETURN.
 *
 * $Id: return.c 3139 2005-10-22 23:50:56Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/return", FALSE, _modinit, _moddeinit,
	"$Id: return.c 3139 2005-10-22 23:50:56Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_return(char *origin);

command_t ns_return = { "RETURN", "Returns a nickname to its owner.",
			AC_IRCOP, ns_cmd_return };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	ns_helptree = module_locate_symbol("nickserv/main", "ns_helptree");
	command_add(&ns_return, ns_cmdtree);
	help_addentry(ns_helptree, "RETURN", "help/nickserv/return", NULL);
}

void _moddeinit()
{
	command_delete(&ns_return, ns_cmdtree);
	help_delentry(ns_helptree, "RETURN");
}

static void ns_cmd_return(char *origin)
{
	char *target = strtok(NULL, " ");
	char *newmail = strtok(NULL, " ");
	char *newpass = gen_pw(12);

	myuser_t *mu;

	if (!target || !newmail)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2RETURN\2.");
		notice(nicksvs.nick, origin, "Usage: RETURN <nickname> <e-mail address>");
		return;
	}

	if (!(mu = myuser_find(target)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", target);
		return;
	}

	if ((strlen(newmail) > 32) || !validemail(newmail))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a valid e-mail address.", newmail);
		return;
	}

	free(mu->email);
	strlcpy(mu->email, newmail, NICKLEN);
	free(mu->pass);
	strlcpy(mu->pass, newpass, NICKLEN);
	sendemail(mu->name, mu->pass, 2);

	/* prevents users from "stealing it back" in the event of a takeover */
	metadata_delete(mu, METADATA_USER, "private:verify:emailchg:key");
	metadata_delete(mu, METADATA_USER, "private:verify:emailchg:newemail");
	metadata_delete(mu, METADATA_USER, "private:verify:emailchg:timestamp");

	wallops("%s returned the nickname \2%s\2 to \2%s\2", origin, target, newmail);
	notice(nicksvs.nick, origin, "The e-mail address for \2%s\2 has been set to \2%s\2",
						target, newmail);
	notice(nicksvs.nick, origin, "A random password has been set; it has been sent to \2%s\2.",
						newmail);
}
