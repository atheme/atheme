/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * XMLRPC account management functions.
 *
 * $Id: account.c 2439 2005-09-28 18:25:05Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"xmlrpc/account", FALSE, _modinit, _moddeinit,
	"$Id: account.c 2439 2005-09-28 18:25:05Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

boolean_t using_nickserv = FALSE;

/*
 * atheme.register_account
 *
 * XML inputs:
 *       account to register, password, email.
 *
 * XML outputs:
 *       fault 1 - account already exists, please try again
 *       fault 2 - password != account, try again
 *       fault 3 - invalid email address
 *       fault 4 - not enough parameters
 *       fault 5 - user is on IRC (would be unfair to claim ownership)
 *       fault 6 - too many accounts associated with this email
 *       default - success message
 *
 * Side Effects:
 *       an account is registered in the system
 */
static int register_account(int parc, char *parv[])
{
	user_t *u;
	myuser_t *mu, *tmu;
	node_t *n;
	uint32_t i, tcnt;
	static char buf[XMLRPC_BUFSIZE];

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(4, "register_account: not enough parameters");
		return 0;
	}

	if (using_nickserv == TRUE && (u = user_find(parv[0])) != NULL)
	{
		xmlrpc_generic_error(5, "a user matching this account is on IRC");
		return 0;
	}

	if (!strcasecmp(parv[0], parv[1]))
	{
		xmlrpc_generic_error(2, "you cannot use your account name as a password.");
		return 0;
	}

	if (!validemail(parv[2]))
	{
		xmlrpc_generic_error(3, "invalid email address");
		return 0;
	}

	if ((mu = myuser_find(parv[0])) != NULL)
	{
		xmlrpc_generic_error(1, "account already exists");
		return 0;
	}

	for (i = 0, tcnt = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			tmu = (myuser_t *)n->data;

			if (!strcasecmp(parv[2], tmu->email))
				tcnt++;
		}
	}

	if (tcnt >= me.maxusers)
	{
		xmlrpc_generic_error(6, "too many accounts registered for this email.");
		return 0;
	}

	snoop("REGISTER: \2%s\2 to \2%s\2 (via \2xmlrpc\2)", parv[0], parv[2]);

	mu = myuser_add(parv[0], parv[1], parv[2]);
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;
	mu->flags |= config_options.defuflags;

	if (me.auth == AUTH_EMAIL)
	{
		char *key = gen_pw(12);
		mu->flags |= MU_WAITAUTH;

		metadata_add(mu, METADATA_USER, "private:verify:register:key", key);
		metadata_add(mu, METADATA_USER, "private:verify:register:timestamp", itoa(time(NULL)));

		xmlrpc_string(buf, "An email containing nickname activiation instructions has been sent to your email address.");
		xmlrpc_string(buf, "If you do not complete registration within one day your nickname will expire.");

		sendemail(mu->name, key, 1);

		free(key);
	}

	xmlrpc_string(buf, "Registration successful.");
	xmlrpc_send(1, buf);
	return 0;
}

static int verify_account(int parc, char *parv[])
{
	return 0;
}

void _modinit(module_t *m)
{
	if (module_find_published("nickserv/main"))
		using_nickserv = TRUE;

	xmlrpc_register_method("atheme.register_account", register_account);
	xmlrpc_register_method("atheme.verify_account", verify_account);	
}

void _moddeinit(void)
{
	xmlrpc_unregister_method("atheme.register_account");
	xmlrpc_unregister_method("atheme.verify_account");
}
