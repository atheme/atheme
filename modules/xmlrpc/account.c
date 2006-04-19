/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * XMLRPC account management functions.
 *
 * $Id: account.c 5115 2006-04-19 12:01:42Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"xmlrpc/account", FALSE, _modinit, _moddeinit,
	"$Id: account.c 5115 2006-04-19 12:01:42Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

boolean_t using_nickserv = FALSE;

/*
 * atheme.account.register
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
 *       fault 7 - invalid account name
 *       fault 8 - invalid password
 *       fault 9 - sending email failed
 *       default - success message
 *
 * Side Effects:
 *       an account is registered in the system
 */
static int account_register(void *conn, int parc, char *parv[])
{
	myuser_t *mu, *tmu;
	node_t *n;
	uint32_t i, tcnt;
	static char buf[XMLRPC_BUFSIZE];

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if (using_nickserv == TRUE && user_find_named(parv[0]))
	{
		xmlrpc_generic_error(5, "A user matching this account is already on IRC.");
		return 0;
	}

	/* This only handles the most basic sanity checking.
	 * We need something better here. Note that unreal has
	 * customizable allowable nickname characters. Ugh.
	 *
	 * It would be great if we could move this into something
	 * like isvalidnick() so it would be easier to reuse --
	 * we'll need to perform a lot of sanity checking on
	 * XML-RPC requests.
	 *
	 *    -- alambert
	 */
	if (using_nickserv == TRUE)
	{
		if (strchr(parv[0], '.') || strchr(parv[0], ' ') || strchr(parv[0], '\n')
			|| strchr(parv[0], '\r') || strchr(parv[0], '$') || strchr(parv[0], ':')
			|| strchr(parv[0], '!') || strchr(parv[0], '#') || strchr(parv[0], ',')
			|| !(strlen(parv[0]) <= (NICKLEN - 1)) || IsDigit(parv[0][0]) || (parv[0][0] == '-'))
		{
			xmlrpc_generic_error(6, "The account name is invalid.");
			return 0;
		}
	}
	else	/* fewer restrictions for account names */
	{
		if (strchr(parv[0], ' ') || strchr(parv[0], '\n') || strchr(parv[0], '\r'))
		{
			xmlrpc_generic_error(6, "The account name is invalid.");
			return 0;
		}
	}

	if (!strcasecmp(parv[0], parv[1]))
	{
		xmlrpc_generic_error(2, "You cannot use your account name as a password.");
		return 0;
	}

	if (strchr(parv[1], ' ') || strchr(parv[1], '\n') || strchr(parv[1], '\r')
		|| !(strlen(parv[1]) <= (NICKLEN - 1)))
	{
		xmlrpc_generic_error(7, "The password is invalid.");
		return 0;
	}

	/* see above comment on sanity-checking */
	if (strchr(parv[2], ' ') || strchr(parv[2], '\n') || strchr(parv[2], '\r')
		|| !validemail(parv[2]) || !(strlen(parv[2]) <= (EMAILLEN - 1)))
	{
		xmlrpc_generic_error(3, "The e-mail address is invalid.");
		return 0;
	}

	if ((mu = myuser_find(parv[0])) != NULL)
	{
		xmlrpc_generic_error(1, "The account is already registered.");
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
		xmlrpc_generic_error(6, "Too many accounts are associated with this e-mail address.");
		return 0;
	}

	snoop("REGISTER: \2%s\2 to \2%s\2 (via \2xmlrpc\2)", parv[0], parv[2]);

	mu = myuser_add(parv[0], parv[1], parv[2], config_options.defuflags);
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;

	if (me.auth == AUTH_EMAIL)
	{
		char *key;
		mu->flags |= MU_WAITAUTH;

		key = gen_pw(12);
		if (!sendemail(using_nickserv ? nicksvs.me->me : usersvs.me->me, EMAIL_REGISTER, mu, key))
		{
			xmlrpc_generic_error(9, "Sending email failed.");
			myuser_delete(mu);
			free(key);
			return 0;
		}

		metadata_add(mu, METADATA_USER, "private:verify:register:key", key);
		free(key);
		metadata_add(mu, METADATA_USER, "private:verify:register:timestamp", itoa(time(NULL)));

		xmlrpc_string(buf, "Registration successful but e-mail activation required within one day.");

		free(key);
	}
	else
		xmlrpc_string(buf, "Registration successful.");

	logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, mu, CMDLOG_REGISTER, "REGISTER to %s", mu->email);

	xmlrpc_send(1, buf);
	return 0;
}

/*
 * atheme.account.verify
 *
 * XML inputs:
 *       authcookie, account name, requested operation, key
 *
 * XML outputs:
 *       fault 1 - the account is not registered
 *       fault 2 - the operation has already been verified
 *       fault 3 - invalid verification key for this operation
 *       fault 4 - insufficient parameters
 *       fault 5 - invalid operation requested
 *       fault 6 - invalid authcookie
 *       default - success
 *
 * Side Effects:
 *       an account-related operation is verified.
 */      
static int account_verify(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	metadata_t *md;
	char buf[XMLRPC_BUFSIZE];

	if (parc < 4)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(1, "The account is not registered.");
		return 0;
	}

	/* avoid information leaks */
	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(6, "Authcookie validation failed.");
		return 0;
	}

	if (!strcasecmp(parv[2], "REGISTER"))
	{
		if (!(mu->flags & MU_WAITAUTH) || !(md = metadata_find(mu, METADATA_USER, "private:verify:register:key")))
		{
			xmlrpc_generic_error(2, "The operation has already been verified.");
			return 0;
		}

		if (!strcasecmp(parv[3], md->value))
		{
			mu->flags &= ~MU_WAITAUTH;

			snoop("REGISTER:VS: \2%s\2 via xmlrpc", mu->email);

			metadata_delete(mu, METADATA_USER, "private:verify:register:key");
			metadata_delete(mu, METADATA_USER, "private:verify:register:timestamp");

			logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, mu, CMDLOG_SET, "VERIFY REGISTER (email: %s)", mu->email);
			xmlrpc_string(buf, "Registration verification was successful.");
			xmlrpc_send(1, buf);
			return 0;
		}

		snoop("REGISTER:VF: \2%s\2 via xmlrpc", mu->email);
		logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, mu, CMDLOG_SET, "failed VERIFY REGISTER (invalid key)");
		xmlrpc_generic_error(3, "Invalid key for this operation.");
		return 0;
	}
	else if (!strcasecmp(parv[2], "EMAILCHG"))
	{
		if (!(md = metadata_find(mu, METADATA_USER, "private:verify:emailchg:key")))
		{
			xmlrpc_generic_error(2, "The operation has already been verified.");
			return 0;
		}

		if (!strcasecmp(parv[3], md->value))
                {
			md = metadata_find(mu, METADATA_USER, "private:verify:emailchg:newemail");

			strlcpy(mu->email, md->value, EMAILLEN);

			snoop("SET:EMAIL:VS: \2%s\2 via xmlrpc", mu->email);
			logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, mu, CMDLOG_SET, "VERIFY EMAILCHG (email: %s)", mu->email);

			metadata_delete(mu, METADATA_USER, "private:verify:emailchg:key");
			metadata_delete(mu, METADATA_USER, "private:verify:emailchg:newemail");
			metadata_delete(mu, METADATA_USER, "private:verify:emailchg:timestamp");

			xmlrpc_string(buf, "E-Mail change verification was successful.");
			xmlrpc_send(1, buf);

			return 0;
                }

		snoop("REGISTER:VF: \2%s\2 via xmlrpc", mu->email);
		logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, mu, CMDLOG_SET, "failed VERIFY EMAILCHG (invalid key)");
		xmlrpc_generic_error(3, "Invalid key for this operation.");

		return 0;
	}
	else
	{
		xmlrpc_generic_error(5, "Invalid verification operation requested.");
		return 0;
	}

	return 0;
}

/*
 * atheme.login
 *
 * XML Inputs:
 *       account name and password
 *
 * XML Outputs:
 *       fault 1 - account is not registered
 *       fault 2 - invalid username and password
 *       fault 4 - insufficient parameters
 *       fault 6 - account is frozen
 *       default - success (authcookie)
 *
 * Side Effects:
 *       an authcookie ticket is created for the myuser_t.
 *       the user's lastlogin is updated
 */
static int do_login(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	authcookie_t *ac;
	char buf[BUFSIZE];

	if (parc < 2)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[0])))
	{
		xmlrpc_generic_error(1, "The account is not registered.");
		return 0;
	}

	if (metadata_find(mu, METADATA_USER, "private:freeze:freezer") != NULL)
	{
		logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, NULL, CMDLOG_LOGIN, "failed LOGIN to %s (frozen)", mu->name);
		xmlrpc_generic_error(6, "The account has been frozen.");
		return 0;
	}

	if (!verify_password(mu, parv[1]))
	{
		logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, NULL, CMDLOG_LOGIN, "failed LOGIN to %s (bad password)", mu->name);
		xmlrpc_generic_error(2, "The password is not valid for this account.");
		return 0;
	}

	mu->lastlogin = CURRTIME;

	ac = authcookie_create(mu);

	logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, mu, CMDLOG_LOGIN, "LOGIN");

	xmlrpc_string(buf, ac->ticket);
	xmlrpc_send(1, buf);

	return 0;
}

/*
 * atheme.logout
 *
 * XML inputs:
 *       authcookie, and account name.
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - unknown authcookie
 *       fault 3 - unknown user
 *       fault 4 - insufficient parameters
 *       default - success message
 *
 * Side Effects:
 *       an authcookie ticket is destroyed.
 */
static int do_logout(void *conn, int parc, char *parv[])
{
	authcookie_t *ac;
	myuser_t *mu;
        char buf[XMLRPC_BUFSIZE];

        if (parc < 2)
        {
                xmlrpc_generic_error(4, "Insufficient parameters.");
                return 0;
        }

        if ((mu = myuser_find(parv[1])) == NULL)
        {
                xmlrpc_generic_error(3, "Unknown user.");
                return 0;
        }

	if ((ac = authcookie_find(parv[0], mu)) == NULL)
	{
		xmlrpc_generic_error(2, "Unknown authcookie.");
		return 0;
	}

        if (authcookie_validate(parv[0], mu) == FALSE)
        {
                xmlrpc_generic_error(1, "Invalid authcookie for this account.");
                return 0;
        }

	logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, mu, CMDLOG_LOGIN, "LOGOUT");

        authcookie_destroy(ac);

        xmlrpc_string(buf, "You are now logged out.");
        xmlrpc_send(1, buf);

        return 0;
}

/*
 * atheme.account.metadata.set
 *
 * XML inputs:
 *       authcookie, account name, key, value
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - unknown account
 *       fault 4 - insufficient parameters
 *       fault 5 - invalid parameters
 *       fault 6 - too many entries
 *       default - success message
 *
 * Side Effects:
 *       metadata is added to an account.
 */ 
static int do_metadata_set(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	char buf[XMLRPC_BUFSIZE];

	if (parc < 4)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if ((mu = myuser_find(parv[1])) == NULL)
	{
		xmlrpc_generic_error(2, "Unknown account.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	if (strchr(parv[2], ':') || (strlen(parv[2]) > 32) || (strlen(parv[3]) > 300)
		|| strchr(parv[2], '\r') || strchr(parv[2], '\n') || strchr(parv[2], ' ')
		|| strchr(parv[3], '\r') || strchr(parv[3], '\n') || strchr(parv[3], ' '))
	{
		xmlrpc_generic_error(5, "Invalid parameters.");
		return 0;
	}

	if (mu->metadata.count >= me.mdlimit)
	{
		xmlrpc_generic_error(6, "Metadata table full.");
		return 0;
	}

	metadata_add(mu, METADATA_USER, parv[2], parv[3]);

	logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, mu, CMDLOG_SET, "SET PROPERTY %s to %s", parv[2], parv[3]);

	xmlrpc_string(buf, "Operation was successful.");
	xmlrpc_send(1, buf);
	return 0;
}

/*
 * atheme.account.metadata.delete
 *
 * XML inputs:
 *       authcookie, account name, key
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - unknown account
 *       fault 4 - insufficient parameters
 *       fault 5 - invalid parameters
 *       fault 6 - key never existed
 *       default - success message
 *
 * Side Effects:
 *       metadata is deleted from an account.
 */ 
static int do_metadata_delete(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	char buf[XMLRPC_BUFSIZE];

	if (parc < 3)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if ((mu = myuser_find(parv[1])) == NULL)
	{
		xmlrpc_generic_error(2, "Unknown account.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	/* don't let them delete private:mark or the like :) */
	if (strchr(parv[2], ':') || (strlen(parv[2]) > 32)
		|| strchr(parv[2], '\r') || strchr(parv[2], '\n') || strchr(parv[2], ' '))
	{
		xmlrpc_generic_error(5, "Invalid parameters.");
		return 0;
	}

	if (!metadata_find(mu, METADATA_USER, parv[2]))
	{
		xmlrpc_generic_error(6, "Key does not exist.");
		return 0;
	}

	metadata_delete(mu, METADATA_USER, parv[2]);

	logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, mu, CMDLOG_SET, "SET PROPERTY %s (deleted)", parv[2]);

	xmlrpc_string(buf, "Operation was successful.");
	xmlrpc_send(1, buf);
	return 0;
}

/*
 * atheme.account.metadata.get
 *
 * XML inputs:
 *       account name, key
 *
 * XML outputs:
 *       fault 1 - unknown account
 *       fault 2 - insufficient parameters
 *       fault 3 - invalid parameters
 *       fault 4 - key doesn't exist
 *       default - value
 *
 * Side Effects:
 *       none.
 */ 
static int do_metadata_get(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	metadata_t *md;
	char buf[XMLRPC_BUFSIZE];

	if (parc < 2)
	{
		xmlrpc_generic_error(2, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[0])))
	{
		xmlrpc_generic_error(1, "Unknown account.");
		return 0;
	}

	if ((strlen(parv[1]) > 32)
		|| strchr(parv[1], '\r') || strchr(parv[1], '\n') || strchr(parv[1], ' '))
	{
		xmlrpc_generic_error(3, "Invalid parameters.");
		return 0;
	}

	/* if private, pretend it doesn't exist */
	if (!(md = metadata_find(mu, METADATA_USER, parv[1])) || md->private)
	{
		xmlrpc_generic_error(4, "Key does not exist.");
		return 0;
	}

	logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, NULL, CMDLOG_GET, "%s GET PROPERTY %s", mu->name, parv[1]);

	xmlrpc_string(buf, md->value);
	xmlrpc_send(1, buf);
	return 0;
}

/*
 * atheme.account.vhost
 *
 * XML inputs:
 *       authcookie, account name, [vhost]
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - unknown account
 *       fault 4 - insufficient parameters
 *       fault 5 - invalid parameters
 *       default - success message
 *
 * Side Effects:
 *       a vhost is set or deleted from an account.
 */ 
static int do_set_vanity_host(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	char buf[XMLRPC_BUFSIZE];

	if (parc < 2)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if ((mu = myuser_find(parv[1])) == NULL)
	{
		xmlrpc_generic_error(2, "Unknown account.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	if (parc > 2)
	{
		/* Never ever allow @!?* as they have special meaning in all ircds */
		if (strchr(parv[2], '@') || strchr(parv[2], '!') ||
			strchr(parv[2], '?') || strchr(parv[2], '*') ||
			strlen(parv[2]) >= HOSTLEN)
		{
			xmlrpc_generic_error(5, "Invalid parameters.");
			return 0;
		}
		/* XXX more checks here, perhaps as a configurable regexp? */
		metadata_add(mu, METADATA_USER, "private:usercloak", parv[2]);
		logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, mu, CMDLOG_ADMIN, "VHOST ASSIGN %s", parv[2]);
	}
	else
	{
		metadata_delete(mu, METADATA_USER, "private:usercloak");
		logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, mu, CMDLOG_ADMIN, "VHOST DELETE");
	}

	xmlrpc_string(buf, "Operation was successful.");
	xmlrpc_send(1, buf);
	return 0;
}

/*
 * atheme.account.set_password
 *
 * XML inputs:
 *       authcookie, account name, new password
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - unknown account
 *       fault 4 - insufficient parameters
 *       fault 6 - passwords do not match
 *       default - success message
 *
 * Side Effects:
 *       an account password is changed.
 */
static int do_set_password(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	char buf[XMLRPC_BUFSIZE];

	if (parc < 3)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if ((mu = myuser_find(parv[1])) == NULL)
	{
		xmlrpc_generic_error(2, "Unknown account.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	set_password(mu, parv[2]);

	logcommand_external(using_nickserv ? nicksvs.me : usersvs.me, "xmlrpc", conn, mu, CMDLOG_ADMIN, "SET PASSWORD");

	xmlrpc_string(buf, "Operation was successful.");
	xmlrpc_send(1, buf);
	return 0;
}

void _modinit(module_t *m)
{
	if (module_find_published("nickserv/main"))
		using_nickserv = TRUE;

	xmlrpc_register_method("atheme.account.register", account_register);
	xmlrpc_register_method("atheme.account.verify", account_verify);	
	xmlrpc_register_method("atheme.login", do_login);
        xmlrpc_register_method("atheme.logout", do_logout);
	xmlrpc_register_method("atheme.account.metadata.set", do_metadata_set);
	xmlrpc_register_method("atheme.account.metadata.delete", do_metadata_delete);
	xmlrpc_register_method("atheme.account.metadata.get", do_metadata_get);
	xmlrpc_register_method("atheme.account.vhost", do_set_vanity_host);
	xmlrpc_register_method("atheme.account.set_password", do_set_password);
}

void _moddeinit(void)
{
	xmlrpc_unregister_method("atheme.account.register");
	xmlrpc_unregister_method("atheme.account.verify");
	xmlrpc_unregister_method("atheme.login");
        xmlrpc_unregister_method("atheme.logout");
	xmlrpc_unregister_method("atheme.account.metadata.set");
	xmlrpc_unregister_method("atheme.account.metadata.delete");
	xmlrpc_unregister_method("atheme.account.metadata.get");
	xmlrpc_unregister_method("atheme.account.vhost");
	xmlrpc_unregister_method("atheme.account.set_password");
}

