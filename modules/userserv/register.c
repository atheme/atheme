/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ REGISTER function.
 *
 * $Id: register.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/register", FALSE, _modinit, _moddeinit,
	"$Id: register.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_register(sourceinfo_t *si, int parc, char *parv[]);

command_t us_register = { "REGISTER", "Registers a services account.", AC_NONE, 3, us_cmd_register };

list_t *us_cmdtree, *us_helptree;

unsigned int tcnt = 0;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_register, us_cmdtree);
	help_addentry(us_helptree, "REGISTER", "help/userserv/register", NULL);
}

void _moddeinit()
{
	command_delete(&us_register, us_cmdtree);
	help_delentry(us_helptree, "REGISTER");
}

static int register_foreach_cb(dictionary_elem_t *delem, void *privdata)
{
	char *email = (char *) privdata;
	myuser_t *tmu = (myuser_t *) delem->node.data;

	if (!strcasecmp(email, tmu->email))
		tcnt++;

	/* optimization: if tcnt >= me.maxusers, quit iterating. -nenolod */
	if (tcnt >= me.maxusers)
		return -1;

	return 0;
}

static void us_cmd_register(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	node_t *n;
	char *account = parv[0];
	char *pass = parv[1];
	char *email = parv[2];
	char lau[BUFSIZE], lao[BUFSIZE];

	if (si->su->myuser)
	{
		notice(usersvs.nick, si->su->nick, "You are already logged in.");
		return;
	}

	if (!account || !pass || !email)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "REGISTER");
		notice(usersvs.nick, si->su->nick, "Syntax: REGISTER <account> <password> <email>");
		return;
	}

	if ((strlen(pass) > 32) || (strlen(email) >= EMAILLEN))
	{
		notice(usersvs.nick, si->su->nick, STR_INVALID_PARAMS, "REGISTER");
		return;
	}

	if (!strcasecmp(pass, account))
	{
		notice(usersvs.nick, si->su->nick, "You cannot use your account name as a password.");
		notice(usersvs.nick, si->su->nick, "Syntax: REGISTER <account> <password> <email>");
		return;
	}

	if (!validemail(email))
	{
		notice(usersvs.nick, si->su->nick, "\2%s\2 is not a valid email address.", email);
		return;
	}

	/* make sure it isn't registered already */
	mu = myuser_find(account);
	if (mu != NULL)
	{
		notice(usersvs.nick, si->su->nick, "\2%s\2 is already registered.", mu->name);

		return;
	}

	/* make sure they're within limits */
	tcnt = 0;
	dictionary_foreach(mulist, register_foreach_cb, email);

	if (tcnt >= me.maxusers)
	{
		notice(usersvs.nick, si->su->nick, "\2%s\2 has too many accounts registered.", email);
		return;
	}

	mu = myuser_add(account, pass, email, config_options.defuflags);
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;

	if (me.auth == AUTH_EMAIL)
	{
		char *key = gen_pw(12);
		mu->flags |= MU_WAITAUTH;

		metadata_add(mu, METADATA_USER, "private:verify:register:key", key);
		metadata_add(mu, METADATA_USER, "private:verify:register:timestamp", itoa(time(NULL)));

		if (!sendemail(si->su, EMAIL_REGISTER, mu, key))
		{
			notice(usersvs.nick, si->su->nick, "Sending email failed, sorry! Registration aborted.");
			myuser_delete(mu);
			free(key);
			return;
		}

		notice(usersvs.nick, si->su->nick, "An email containing account activation instructions has been sent to \2%s\2.", mu->email);
		notice(usersvs.nick, si->su->nick, "If you do not complete registration within one day your account will expire.");

		free(key);
	}

	si->su->myuser = mu;
	n = node_create();
	node_add(si->su, n, &mu->logins);

	if (!(mu->flags & MU_WAITAUTH))
		/* only grant ircd registered status if it's verified */
		ircd_on_login(si->su->nick, mu->name, NULL);

	snoop("REGISTER: \2%s\2 to \2%s\2", account, email);
	logcommand(usersvs.me, si->su, CMDLOG_REGISTER, "REGISTER to %s", email);
	if (is_soper(mu))
	{
		wallops("%s registered the account \2%s\2 and gained services operator privileges.", si->su->nick, mu->name);
		snoop("SOPER: \2%s\2 as \2%s\2", si->su->nick, mu->name);
	}

	notice(usersvs.nick, si->su->nick, "\2%s\2 is now registered to \2%s\2.", mu->name, mu->email);
	notice(usersvs.nick, si->su->nick, "The password is \2%s\2. Please write this down for future reference.", pass);
	hook_call_event("user_register", mu);

	snprintf(lau, BUFSIZE, "%s@%s", si->su->user, si->su->vhost);
	metadata_add(mu, METADATA_USER, "private:host:vhost", lau);

	snprintf(lao, BUFSIZE, "%s@%s", si->su->user, si->su->host);
	metadata_add(mu, METADATA_USER, "private:host:actual", lao);

}
