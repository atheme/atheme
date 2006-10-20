/*
 * Copyright (c) 2006 Atheme developers
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Account-related functions.
 *
 * $Id: account.c 6715 2006-10-20 18:31:20Z nenolod $
 */

#include "atheme.h"

dictionary_tree_t *mulist;

/*
 * init_accounts()
 *
 * Initializes the accounts DTree.
 *
 * Inputs:
 *      - none
 *
 * Outputs:
 *      - none
 *
 * Side Effects:
 *      - if the DTree initialization fails, services will abort.
 */
void init_accounts(void)
{
	mulist = dictionary_create("myuser", HASH_USER, irccasecmp);
}

/*
 * myuser_add(char *name, char *pass, char *email, uint32_t flags)
 *
 * Creates an account and adds it to the accounts DTree.
 *
 * Inputs:
 *      - an account name
 *      - an account password
 *      - an email to associate with the account or NONE
 *      - flags for the account
 *
 * Outputs:
 *      - on success, a new myuser_t object (account)
 *      - on failure, NULL.
 *
 * Side Effects:
 *      - the created account is added to the accounts DTree,
 *        this may be undesirable for a factory.
 */
myuser_t *myuser_add(char *name, char *pass, char *email, uint32_t flags)
{
	myuser_t *mu;
	node_t *n;
	soper_t *soper;

	mu = myuser_find(name);

	if (mu)
	{
		slog(LG_DEBUG, "myuser_add(): myuser already exists: %s", name);
		return mu;
	}

	slog(LG_DEBUG, "myuser_add(): %s -> %s", name, email);

	n = node_create();
	mu = BlockHeapAlloc(myuser_heap);

	/* set the password later */
	strlcpy(mu->name, name, NICKLEN);
	strlcpy(mu->email, email, EMAILLEN);
	mu->registered = CURRTIME;
	mu->flags = flags;

	/* If it's already crypted, don't touch the password. Otherwise,
	 * use set_password() to initialize it. Why? Because set_password
	 * will move the user to encrypted passwords if possible. That way,
	 * new registers are immediately protected and the database is
	 * immediately converted the first time we start up with crypto.
	 */
	if (flags & MU_CRYPTPASS)
		strlcpy(mu->pass, pass, NICKLEN);
	else
		set_password(mu, pass);

	dictionary_add(mulist, mu->name, mu);

	if ((soper = soper_find_named(mu->name)) != NULL)
	{
		slog(LG_DEBUG, "myuser_add(): user `%s' has been declared as soper, activating privileges.", mu->name);
		soper->myuser = mu;
		mu->soper = soper;
	}

	cnt.myuser++;

	return mu;
}

/*
 * myuser_delete(myuser_t *mu)
 *
 * Destroys and removes an account from the accounts DTree.
 *
 * Inputs:
 *      - account to destroy
 *
 * Outputs:
 *      - nothing
 *
 * Side Effects:
 *      - an account is destroyed and removed from the accounts DTree.
 */
void myuser_delete(myuser_t *mu)
{
	myuser_t *successor;
	mychan_t *mc;
	chanacs_t *ca;
	user_t *u;
	node_t *n, *tn;
	metadata_t *md;
	uint32_t i;

	if (!mu)
	{
		slog(LG_DEBUG, "myuser_delete(): called for NULL myuser");
		return;
	}

	slog(LG_DEBUG, "myuser_delete(): %s", mu->name);

	/* log them out */
	if (authservice_loaded)
	{
		LIST_FOREACH_SAFE(n, tn, mu->logins.head)
		{
			u = (user_t *)n->data;
			if (!ircd_on_logout(u->nick, mu->name, NULL))
			{
				u->myuser = NULL;
				node_del(n, &mu->logins);
				node_free(n);
			}
		}
	}

	/* kill all their channels
	 *
	 * We CANNOT do this based soley on chanacs!
	 * A founder could remove all of his flags.
	 */
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH_SAFE(n, tn, mclist[i].head)
		{
			mc = (mychan_t *)n->data;

			/* attempt succession */
			if (mc->founder == mu && (successor = mychan_pick_successor(mc)) != NULL)
			{
				snoop("SUCCESSION: \2%s\2 -> \2%s\2 from \2%s\2", successor->name, mc->name, mc->founder->name);
				if (chansvs.me != NULL)
					verbose(mc, "Foundership changed to \2%s\2 because \2%s\2 was dropped.", successor->name, mc->founder->name);

				chanacs_change_simple(mc, successor, NULL, CA_FOUNDER_0, 0, CA_ALL);
				mc->founder = successor;

				if (chansvs.me != NULL)
					myuser_notice(chansvs.nick, mc->founder, "You are now founder on \2%s\2 (as \2%s\2).", mc->name, mc->founder->name);
			}

			/* no successor found */
			if (mc->founder == mu)
			{
				snoop("DELETE: \2%s\2 from \2%s\2", mc->name, mu->name);

				hook_call_event("channel_drop", mc);
				if ((config_options.chan && irccasecmp(mc->name, config_options.chan)) || !config_options.chan)
					part(mc->name, chansvs.nick);
				mychan_delete(mc->name);
			}
		}
	}

	/* remove their chanacs shiz */
	LIST_FOREACH_SAFE(n, tn, mu->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		chanacs_delete(ca->mychan, ca->myuser, ca->level);
	}

	/* remove them from the soper list */
	if (soper_find(mu))
		soper_delete(mu->soper);

	/* delete the metadata */
	LIST_FOREACH_SAFE(n, tn, mu->metadata.head)
	{
		md = (metadata_t *) n->data;
		metadata_delete(mu, METADATA_USER, md->name);
	}

	/* kill any authcookies */
	authcookie_destroy_all(mu);

	/* mu->name is the index for this dtree */
	dictionary_delete(mulist, mu->name);

	BlockHeapFree(myuser_heap, mu);

	cnt.myuser--;
}

/*
 * myuser_find(const char *name)
 *
 * Retrieves an account from the accounts DTree.
 *
 * Inputs:
 *      - account name to retrieve
 *
 * Outputs:
 *      - account wanted or NULL if it's not in the DTree.
 *
 * Side Effects:
 *      - none
 */
myuser_t *myuser_find(const char *name)
{
	return dictionary_retrieve(mulist, name);
}

/*
 * myuser_find_ext(const char *name)
 *
 * Same as myuser_find() but with undernet-style `=nick' expansion.
 *
 * Inputs:
 *      - account name to retrieve or =nick notation for wanted account.
 *
 * Outputs:
 *      - account wanted or NULL if it's not in the DTree.
 *
 * Side Effects:
 *      - none
 */ 
myuser_t *myuser_find_ext(const char *name)
{
	user_t *u;

	if (name == NULL)
		return NULL;

	if (*name == '=')
	{
		u = user_find(name + 1);
		return u != NULL ? u->myuser : NULL;
	}
	else
		return myuser_find(name);
}

/*
 * myuser_notice(char *from, myuser_t *target, char *fmt, ...)
 *
 * Sends a notice to all users logged into an account.
 *
 * Inputs:
 *      - source of message
 *      - target account
 *      - format of message
 *      - zero+ arguments for the formatter
 *
 * Outputs:
 *      - nothing
 *
 * Side Effects:
 *      - a notice is sent to all users logged into the account.
 */
void myuser_notice(char *from, myuser_t *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	node_t *n;
	user_t *u;

	if (target == NULL)
		return;

	/* have to reformat it here, can't give a va_list to notice() :(
	 * -- jilles
	 */
	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	LIST_FOREACH(n, target->logins.head)
	{
		u = (user_t *)n->data;
		notice(from, u->nick, "%s", buf);
	}
}

/*
 * myuser_access_verify()
 *
 * Inputs:
 *     - user to verify, account to verify against
 *
 * Outputs:
 *     - TRUE if user matches an accesslist entry
 *     - FALSE otherwise
 *
 * Side Effects:
 *     - none
 */
boolean_t
myuser_access_verify(user_t *u, myuser_t *mu)
{
	node_t *n;
	char buf[BUFSIZE], buf2[BUFSIZE];

	if (u == NULL || mu == NULL)
	{
		slog(LG_DEBUG, "myuser_access_verify(): invalid parameters: u = %p, mu = %p", u, mu);
		return FALSE;
	}

	snprintf(buf, BUFSIZE, "%s!%s@%s", u->nick, u->user, u->vhost);
	snprintf(buf2, BUFSIZE, "%s!%s@%s", u->nick, u->user, u->host);

	LIST_FOREACH(n, mu->access_list.head)
	{
		char *entry = (char *) n->data;

		if (!match(entry, buf) || !match(entry, buf2))
			return TRUE;
	}

	return FALSE;
}

/*
 * myuser_access_attach()
 *
 * Inputs:
 *     - account to attach access mask to, access mask itself
 *
 * Outputs:
 *     - FALSE: me.mdlimit is reached (too many access entries)
 *     - TRUE : success
 *
 * Side Effects:
 *     - an access mask is added to an account.
 */
boolean_t
myuser_access_attach(myuser_t *mu, char *mask)
{
	node_t *n;
	char *msk;

	if (mu == NULL || mask == NULL)
	{
		slog(LG_DEBUG, "myuser_access_attach(): invalid parameters: mu = %p, mask = %p", mu, mask);
		return FALSE;
	}

	if (LIST_LENGTH(&mu->access_list) > me.mdlimit)
	{
		slog(LG_DEBUG, "myuser_access_attach(): access entry limit reached for %s", mu->name);
		return FALSE;
	}

	msk = sstrdup(mask);
	n = node_create();
	node_add(msk, n, &mu->access_list);

	return TRUE;
}

/*
 * myuser_access_delete()
 *
 * Inputs:
 *     - account to delete access mask from, access mask itself
 *
 * Outputs:
 *     - FALSE: me.mdlimit is reached (too many access entries)
 *     - TRUE : success
 *
 * Side Effects:
 *     - an access mask is added to an account.
 */
void
myuser_access_delete(myuser_t *mu, char *mask)
{
	node_t *n, *tn;

	if (mu == NULL || mask == NULL)
	{
		slog(LG_DEBUG, "myuser_access_delete(): invalid parameters: mu = %p, mask = %p", mu, mask);
		return;
	}

	LIST_FOREACH_SAFE(n, tn, mu->access_list.head)
	{
		char *entry = (char *) n->data;

		if (!strcasecmp(entry, mask))
		{
			node_del(n, &n->data);
			free(entry);

			return;
		}
	}
}
