/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ REGISTER function.
 *
 * $Id: register.c 3685 2005-11-09 01:07:04Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/register", FALSE, _modinit, _moddeinit,
	"$Id: register.c 3685 2005-11-09 01:07:04Z alambert $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_register(char *origin);

command_t ns_register = { "REGISTER", "Registers a nickname.", AC_NONE, ns_cmd_register };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	ns_helptree = module_locate_symbol("nickserv/main", "ns_helptree");
	command_add(&ns_register, ns_cmdtree);
	help_addentry(ns_helptree, "REGISTER", "help/nickserv/register", NULL);
}

void _moddeinit()
{
	command_delete(&ns_register, ns_cmdtree);
	help_delentry(ns_helptree, "REGISTER");
}

static void ns_cmd_register(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu, *tmu;
	node_t *n;
	char *pass = strtok(NULL, " ");
	char *email = strtok(NULL, " ");
	char lau[BUFSIZE], lao[BUFSIZE];
	uint32_t i, tcnt;

	if (u->myuser)
	{
		notice(nicksvs.nick, origin, "You are already logged in.");
		return;
	}

	if (!pass || !email)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters specified for \2REGISTER\2.");
		notice(nicksvs.nick, origin, "Syntax: REGISTER <password> <email>");
		return;
	}

	if ((strlen(pass) > 32) || (strlen(email) >= EMAILLEN))
	{
		notice(nicksvs.nick, origin, "Invalid parameters specified for \2REGISTER\2.");
		return;
	}

	if (IsDigit(*u->nick))
	{
		notice(nicksvs.nick, origin, "For security reasons, you can't register your UID.");
		notice(nicksvs.nick, origin, "Please change to a real nickname, and try again.");
		return;
	}

	if (!strcasecmp(pass, origin))
	{
		notice(nicksvs.nick, origin, "You cannot use your nickname as a password.");
		notice(nicksvs.nick, origin, "Syntax: REGISTER <password> <email>");
		return;
	}

	if (!validemail(email))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a valid email address.", email);
		return;
	}

	/* make sure it isn't registered already */
	mu = myuser_find(origin);
	if (mu != NULL)
	{
		/* should we reveal the e-mail address? (from ns_info.c) */
		if (!(mu->flags & MU_HIDEMAIL)
			|| (is_sra(u->myuser) || is_ircop(u) || u->myuser == mu))
			notice(nicksvs.nick, origin, "\2%s\2 is already registered to \2%s\2.", mu->name, mu->email);
		else
			notice(nicksvs.nick, origin, "\2%s\2 is already registered.", mu->name);

		return;
	}

	/* make sure they're within limits */
	for (i = 0, tcnt = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			tmu = (myuser_t *)n->data;

			if (!strcasecmp(email, tmu->email))
				tcnt++;
		}
	}

	if (tcnt >= me.maxusers)
	{
		notice(nicksvs.nick, origin, "\2%s\2 has too many nicknames registered.", email);
		return;
	}

	mu = myuser_add(origin, pass, email, config_options.defuflags);
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;

	if (me.auth == AUTH_EMAIL)
	{
		char *key = gen_pw(12);
		mu->flags |= MU_WAITAUTH;

		metadata_add(mu, METADATA_USER, "private:verify:register:key", key);
		metadata_add(mu, METADATA_USER, "private:verify:register:timestamp", itoa(time(NULL)));

		if (!sendemail(u, EMAIL_REGISTER, mu, key))
		{
			notice(nicksvs.nick, origin, "Sending email failed, sorry! Registration aborted.");
			myuser_delete(mu->name);
			free(key);
			return;
		}

		notice(nicksvs.nick, origin, "An email containing nickname activiation instructions has been sent to \2%s\2.", mu->email);
		notice(nicksvs.nick, origin, "If you do not complete registration within one day your nickname will expire.");

		free(key);
	}

	u->myuser = mu;
	n = node_create();
	node_add(u, n, &mu->logins);

	if (!(mu->flags & MU_WAITAUTH))
		/* only grant ircd registered status if it's verified */
		ircd_on_login(origin, mu->name, NULL);

	snoop("REGISTER: \2%s\2 to \2%s\2", origin, email);
	logcommand(nicksvs.me, u, CMDLOG_REGISTER, "REGISTER to %s", email);

	notice(nicksvs.nick, origin, "\2%s\2 is now registered to \2%s\2.", mu->name, mu->email);
	notice(nicksvs.nick, origin, "The password is \2%s\2. Please write this down for future reference.", pass);
	hook_call_event("user_register", mu);

	/* keep track of login address for users */
	strlcpy(lau, u->user, BUFSIZE);
	strlcat(lau, "@", BUFSIZE);
	strlcat(lau, u->vhost, BUFSIZE);
	metadata_add(mu, METADATA_USER, "private:host:vhost", lau);
	/* and for opers */
	strlcpy(lao, u->user, BUFSIZE);
	strlcat(lao, "@", BUFSIZE);
	strlcat(lao, u->host, BUFSIZE);
	metadata_add(mu, METADATA_USER, "private:host:actual", lao);


}
