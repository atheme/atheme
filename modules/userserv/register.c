/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ REGISTER function.
 *
 * $Id: register.c 2359 2005-09-25 02:49:10Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/register", FALSE, _modinit, _moddeinit,
	"$Id: register.c 2359 2005-09-25 02:49:10Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_register(char *origin);

command_t us_register = { "REGISTER", "Registers a nickname.", AC_NONE, us_cmd_register };

list_t *us_cmdtree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	command_add(&us_register, us_cmdtree);
}

void _moddeinit()
{
	command_delete(&us_register, us_cmdtree);
}

static void us_cmd_register(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu, *tmu;
	node_t *n;
	char *pass = strtok(NULL, " ");
	char *email = strtok(NULL, " ");
	uint32_t i, tcnt;

	if (u->myuser)
	{
		notice(usersvs.nick, origin, "You are already logged in.");
		return;
	}

	if (!pass || !email)
	{
		notice(usersvs.nick, origin, "Insufficient parameters specified for \2REGISTER\2.");
		notice(usersvs.nick, origin, "Syntax: REGISTER <password> <email>");
		return;
	}

	if ((strlen(pass) > 32) || (strlen(email) >= EMAILLEN))
	{
		notice(usersvs.nick, origin, "Invalid parameters specified for \2REGISTER\2.");
		return;
	}

	if (!strcasecmp(pass, origin))
	{
		notice(usersvs.nick, origin, "You cannot use your nickname as a password.");
		notice(usersvs.nick, origin, "Syntax: REGISTER <password> <email>");
		return;
	}

	if (!validemail(email))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not a valid email address.", email);
		return;
	}

	/* make sure it isn't registered already */
	mu = myuser_find(origin);
	if (mu != NULL)
	{
		/* should we reveal the e-mail address? (from us_info.c) */
		if (!(mu->flags & MU_HIDEMAIL)
			|| (is_sra(u->myuser) || is_ircop(u) || u->myuser == mu))
			notice(usersvs.nick, origin, "\2%s\2 is already registered to \2%s\2.", mu->name, mu->email);
		else
			notice(usersvs.nick, origin, "\2%s\2 is already registered.", mu->name);

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
		notice(usersvs.nick, origin, "\2%s\2 has too many nicknames registered.", email);
		return;
	}

	snoop("REGISTER: \2%s\2 to \2%s\2", origin, email);

	mu = myuser_add(origin, pass, email);

	u->myuser = mu;
	n = node_create();
	node_add(u, n, &mu->logins);
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;
	mu->flags |= config_options.defuflags;

	if (me.auth == AUTH_EMAIL)
	{
		char *key = gen_pw(12);
		mu->flags |= MU_WAITAUTH;

		metadata_add(mu, METADATA_USER, "private:verify:register:key", key);
		metadata_add(mu, METADATA_USER, "private:verify:register:timestamp", itoa(time(NULL)));

		notice(usersvs.nick, origin, "An email containing nickname activiation instructions has been sent to \2%s\2.", mu->email);
		notice(usersvs.nick, origin, "If you do not complete registration within one day your nickname will expire.");

		sendemail(mu->name, key, 1);

		free(key);
	}
	else /* only grant ircd registered status if it's verified */
		ircd_on_login(origin, mu->name, NULL);

	notice(usersvs.nick, origin, "\2%s\2 is now registered to \2%s\2.", mu->name, mu->email);
	notice(usersvs.nick, origin, "The password is \2%s\2. Please write this down for future reference.", mu->pass);
	hook_call_event("user_register", mu);
}
