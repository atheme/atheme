/*
 * Copyright (c) 2005-2006 Atheme developers
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Account-related functions.
 *
 * $Id: account.c 7317 2006-12-05 00:14:26Z jilles $
 */

#include "atheme.h"
#include "uplink.h" /* XXX, for sendq_flush(curr_uplink->conn); */

dictionary_tree_t *mulist;
dictionary_tree_t *nicklist;
dictionary_tree_t *mclist;

static BlockHeap *myuser_heap;  /* HEAP_USER */
static BlockHeap *mynick_heap;  /* HEAP_USER */
static BlockHeap *mychan_heap;	/* HEAP_CHANNEL */
static BlockHeap *chanacs_heap;	/* HEAP_CHANACS */
static BlockHeap *metadata_heap;	/* HEAP_CHANUSER */

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
	myuser_heap = BlockHeapCreate(sizeof(myuser_t), HEAP_USER);
	mynick_heap = BlockHeapCreate(sizeof(myuser_t), HEAP_USER);
	mychan_heap = BlockHeapCreate(sizeof(mychan_t), HEAP_CHANNEL);
	chanacs_heap = BlockHeapCreate(sizeof(chanacs_t), HEAP_CHANUSER);
	metadata_heap = BlockHeapCreate(sizeof(metadata_t), HEAP_CHANUSER);

	if (myuser_heap == NULL || mynick_heap == NULL || mychan_heap == NULL
			|| chanacs_heap == NULL || metadata_heap == NULL)
	{
		slog(LG_ERROR, "init_accounts(): block allocator failure.");
		exit(EXIT_FAILURE);
	}

	mulist = dictionary_create("myuser", HASH_USER, irccasecmp);
	nicklist = dictionary_create("mynick", HASH_USER, irccasecmp);
	mclist = dictionary_create("mychan", HASH_CHANNEL, irccasecmp);
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
 *
 * Caveats:
 *      - if nicksvs.no_nick_ownership is not enabled, the caller is
 *        responsible for adding a nick with the same name
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
	mynick_t *mn;
	chanacs_t *ca;
	user_t *u;
	node_t *n, *tn;
	metadata_t *md;
	dictionary_iteration_state_t state;

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
	DICTIONARY_FOREACH(mc, &state, mclist)
	{
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

	/* delete access entries */
	LIST_FOREACH_SAFE(n, tn, mu->access_list.head)
		myuser_access_delete(mu, (char *)n->data);

	/* delete their nicks */
	LIST_FOREACH_SAFE(n, tn, mu->nicks.head)
	{
		mn = (mynick_t *)n->data;
		mynick_delete(mn);
	}

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
 * Same as myuser_find() but with nick group support and undernet-style
 * `=nick' expansion.
 *
 * Inputs:
 *      - account name/nick to retrieve or =nick notation for wanted account.
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
	mynick_t *mn;

	if (name == NULL)
		return NULL;

	if (*name == '=')
	{
		u = user_find_named(name + 1);
		return u != NULL ? u->myuser : NULL;
	}
	else if (nicksvs.no_nick_ownership)
		return myuser_find(name);
	else
	{
		mn = mynick_find(name);
		return mn != NULL ? mn->owner : NULL;
	}
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
 *     - last login time updated if user matches
 */
boolean_t
myuser_access_verify(user_t *u, myuser_t *mu)
{
	node_t *n;
	char buf[USERLEN+HOSTLEN];
	char buf2[USERLEN+HOSTLEN];
	char buf3[USERLEN+HOSTLEN];

	if (u == NULL || mu == NULL)
	{
		slog(LG_DEBUG, "myuser_access_verify(): invalid parameters: u = %p, mu = %p", u, mu);
		return FALSE;
	}

	if (!use_myuser_access)
		return FALSE;

	snprintf(buf, sizeof buf, "%s@%s", u->user, u->vhost);
	snprintf(buf2, sizeof buf2, "%s@%s", u->user, u->host);
	snprintf(buf3, sizeof buf3, "%s@%s", u->user, u->ip);

	LIST_FOREACH(n, mu->access_list.head)
	{
		char *entry = (char *) n->data;

		if (!match(entry, buf) || !match(entry, buf2) || !match(entry, buf3) || !match_cidr(entry, buf3))
		{
			mu->lastlogin = CURRTIME;
			return TRUE;
		}
	}

	return FALSE;
}

/*
 * myuser_access_add()
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
myuser_access_add(myuser_t *mu, char *mask)
{
	node_t *n;
	char *msk;

	if (mu == NULL || mask == NULL)
	{
		slog(LG_DEBUG, "myuser_access_add(): invalid parameters: mu = %p, mask = %p", mu, mask);
		return FALSE;
	}

	if (LIST_LENGTH(&mu->access_list) > me.mdlimit)
	{
		slog(LG_DEBUG, "myuser_access_add(): access entry limit reached for %s", mu->name);
		return FALSE;
	}

	msk = sstrdup(mask);
	n = node_create();
	node_add(msk, n, &mu->access_list);

	cnt.myuser_access++;

	return TRUE;
}

/*
 * myuser_access_find()
 *
 * Inputs:
 *     - account to find access mask in, access mask
 *
 * Outputs:
 *     - pointer to found access mask or NULL if not found
 *
 * Side Effects:
 *     - none
 */
char *
myuser_access_find(myuser_t *mu, char *mask)
{
	node_t *n;

	if (mu == NULL || mask == NULL)
	{
		slog(LG_DEBUG, "myuser_access_find(): invalid parameters: mu = %p, mask = %p", mu, mask);
		return NULL;
	}

	LIST_FOREACH(n, mu->access_list.head)
	{
		char *entry = (char *) n->data;

		if (!strcasecmp(entry, mask))
			return entry;
	}
	return NULL;
}

/*
 * myuser_access_delete()
 *
 * Inputs:
 *     - account to delete access mask from, access mask itself
 *
 * Outputs:
 *     - none
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
			node_del(n, &mu->access_list);
			free(entry);

			cnt.myuser_access--;

			return;
		}
	}
}

/***************
 * M Y N I C K *
 ***************/

/*
 * mynick_add(myuser_t *mu, const char *name)
 *
 * Creates a nick registration for the given account and adds it to the
 * nicks DTree.
 *
 * Inputs:
 *      - an account pointer
 *      - a nickname
 *
 * Outputs:
 *      - on success, a new mynick_t object
 *      - on failure, NULL.
 *
 * Side Effects:
 *      - the created nick is added to the nick DTree and to the
 *        account's list.
 */
mynick_t *mynick_add(myuser_t *mu, const char *name)
{
	mynick_t *mn;

	mn = mynick_find(name);
	if (mn)
	{
		slog(LG_DEBUG, "mynick_add(): mynick already exists: %s", name);
		return mn;
	}

	slog(LG_DEBUG, "mynick_add(): %s -> %s", name, mu->name);

	mn = BlockHeapAlloc(mynick_heap);

	strlcpy(mn->nick, name, NICKLEN);
	mn->owner = mu;
	mn->registered = CURRTIME;

	dictionary_add(nicklist, mn->nick, mn);
	node_add(mn, &mn->node, &mu->nicks);

	cnt.mynick++;

	return mn;
}

/*
 * mynick_delete(mynick_t *mn)
 *
 * Destroys and removes a nick from the nicks DTree and the account.
 *
 * Inputs:
 *      - nick to destroy
 *
 * Outputs:
 *      - nothing
 *
 * Side Effects:
 *      - a nick is destroyed and removed from the nicks DTree and the account.
 */
void mynick_delete(mynick_t *mn)
{
	if (!mn)
	{
		slog(LG_DEBUG, "mynick_delete(): called for NULL myuser");
		return;
	}

	slog(LG_DEBUG, "mynick_delete(): %s", mn->nick);

	dictionary_delete(nicklist, mn->nick);
	node_del(&mn->node, &mn->owner->nicks);

	BlockHeapFree(mynick_heap, mn);

	cnt.mynick--;
}

/*
 * mynick_find(const char *name)
 *
 * Retrieves a nick from the nicks DTree.
 *
 * Inputs:
 *      - nickname to retrieve
 *
 * Outputs:
 *      - nick wanted or NULL if it's not in the DTree.
 *
 * Side Effects:
 *      - none
 */
mynick_t *mynick_find(const char *name)
{
	return dictionary_retrieve(nicklist, name);
}

/***************
 * M Y C H A N *
 ***************/

mychan_t *mychan_add(char *name)
{
	mychan_t *mc;

	mc = mychan_find(name);

	if (mc)
	{
		slog(LG_DEBUG, "mychan_add(): mychan already exists: %s", name);
		return mc;
	}

	slog(LG_DEBUG, "mychan_add(): %s", name);

	mc = BlockHeapAlloc(mychan_heap);

	strlcpy(mc->name, name, CHANNELLEN);
	mc->founder = NULL;
	mc->registered = CURRTIME;
	mc->chan = channel_find(name);

	dictionary_add(mclist, mc->name, mc);

	cnt.mychan++;

	return mc;
}

void mychan_delete(char *name)
{
	mychan_t *mc = mychan_find(name);
	chanacs_t *ca;
	node_t *n, *tn;

	if (!mc)
	{
		slog(LG_DEBUG, "mychan_delete(): called for nonexistant mychan: %s", name);
		return;
	}

	slog(LG_DEBUG, "mychan_delete(): %s", mc->name);

	/* remove the chanacs shiz */
	LIST_FOREACH_SAFE(n, tn, mc->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (ca->myuser)
			chanacs_delete(ca->mychan, ca->myuser, ca->level);
		else
			chanacs_delete_host(ca->mychan, ca->host, ca->level);
	}

	dictionary_delete(mclist, mc->name);

	BlockHeapFree(mychan_heap, mc);

	cnt.mychan--;
}

mychan_t *mychan_find(const char *name)
{
	return dictionary_retrieve(mclist, name);
}

/* Check if there is anyone on the channel fulfilling the conditions.
 * Fairly expensive, but this is sometimes necessary to avoid
 * inappropriate drops. -- jilles */
boolean_t mychan_isused(mychan_t *mc)
{
	node_t *n;
	channel_t *c;
	chanuser_t *cu;

	c = mc->chan;
	if (c == NULL)
		return FALSE;
	LIST_FOREACH(n, c->members.head)
	{
		cu = n->data;
		if (chanacs_user_flags(mc, cu->user) & CA_USEDUPDATE)
			return TRUE;
	}
	return FALSE;
}

/* Find a user fulfilling the conditions who can take another channel */
myuser_t *mychan_pick_candidate(mychan_t *mc, uint32_t minlevel, int maxtime)
{
	int tcnt;
	node_t *n;
	chanacs_t *ca;
	mychan_t *tmc;
	myuser_t *mu;
	dictionary_iteration_state_t state;

	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;
		if (ca->level & CA_AKICK)
			continue;
		mu = ca->myuser;
		if (mu == NULL || mu == mc->founder)
			continue;
		if ((ca->level & minlevel) == minlevel && (maxtime == 0 || LIST_LENGTH(&mu->logins) > 0 || CURRTIME - mu->lastlogin < maxtime))
		{
			if (has_priv_myuser(mu, PRIV_REG_NOLIMIT))
				return mu;
			tcnt = 0;
			DICTIONARY_FOREACH(tmc, &state, mclist)
			{
				if (is_founder(tmc, mu))
					tcnt++;
			}
		
			if (tcnt < me.maxchans)
				return mu;
		}
	}
	return NULL;
}

/* Pick a suitable successor
 * Note: please do not make this dependent on currently being in
 * the channel or on IRC; this would give an unfair advantage to
 * 24*7 clients and bots.
 * -- jilles */
myuser_t *mychan_pick_successor(mychan_t *mc)
{
	myuser_t *mu;

	/* full privs? */
	mu = mychan_pick_candidate(mc, CA_FOUNDER_0, 7*86400);
	if (mu != NULL)
		return mu;
	mu = mychan_pick_candidate(mc, CA_FOUNDER_0, 0);
	if (mu != NULL)
		return mu;
	/* someone with +R then? (old successor has this, but not sop) */
	mu = mychan_pick_candidate(mc, CA_RECOVER, 7*86400);
	if (mu != NULL)
		return mu;
	mu = mychan_pick_candidate(mc, CA_RECOVER, 0);
	if (mu != NULL)
		return mu;
	/* an op perhaps? */
	mu = mychan_pick_candidate(mc, CA_OP, 7*86400);
	if (mu != NULL)
		return mu;
	mu = mychan_pick_candidate(mc, CA_OP, 0);
	if (mu != NULL)
		return mu;
	/* just an active user with access */
	mu = mychan_pick_candidate(mc, 0, 7*86400);
	if (mu != NULL)
		return mu;
	/* ok you can't say we didn't try */
	return mychan_pick_candidate(mc, 0, 0);
}

/*****************
 * C H A N A C S *
 *****************/

chanacs_t *chanacs_add(mychan_t *mychan, myuser_t *myuser, uint32_t level)
{
	chanacs_t *ca;
	node_t *n1;
	node_t *n2;

	if (!mychan || !myuser)
	{
		slog(LG_DEBUG, "chanacs_add(): got mychan == NULL or myuser == NULL, ignoring");
		return NULL;
	}

	if (*mychan->name != '#')
	{
		slog(LG_DEBUG, "chanacs_add(): got non #channel: %s", mychan->name);
		return NULL;
	}

	slog(LG_DEBUG, "chanacs_add(): %s -> %s", mychan->name, myuser->name);

	n1 = node_create();
	n2 = node_create();

	ca = BlockHeapAlloc(chanacs_heap);

	ca->mychan = mychan;
	ca->myuser = myuser;
	ca->level = level & CA_ALL;

	node_add(ca, n1, &mychan->chanacs);
	node_add(ca, n2, &myuser->chanacs);

	cnt.chanacs++;

	return ca;
}

chanacs_t *chanacs_add_host(mychan_t *mychan, char *host, uint32_t level)
{
	chanacs_t *ca;
	node_t *n;

	if (!mychan || !host)
	{
		slog(LG_DEBUG, "chanacs_add_host(): got mychan == NULL or host == NULL, ignoring");
		return NULL;
	}

	if (*mychan->name != '#')
	{
		slog(LG_DEBUG, "chanacs_add_host(): got non #channel: %s", mychan->name);
		return NULL;
	}

	slog(LG_DEBUG, "chanacs_add_host(): %s -> %s", mychan->name, host);

	n = node_create();

	ca = BlockHeapAlloc(chanacs_heap);

	ca->mychan = mychan;
	ca->myuser = NULL;
	strlcpy(ca->host, host, HOSTLEN);
	ca->level |= level;

	node_add(ca, n, &mychan->chanacs);

	cnt.chanacs++;

	return ca;
}

void chanacs_delete(mychan_t *mychan, myuser_t *myuser, uint32_t level)
{
	chanacs_t *ca;
	node_t *n, *tn, *n2;

	if (!mychan || !myuser)
	{
		slog(LG_DEBUG, "chanacs_delete(): got mychan == NULL or myuser == NULL, ignoring");
		return;
	}

	LIST_FOREACH_SAFE(n, tn, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if ((ca->myuser == myuser) && (ca->level == level))
		{
			slog(LG_DEBUG, "chanacs_delete(): %s -> %s", ca->mychan->name, ca->myuser->name);
			node_del(n, &mychan->chanacs);
			node_free(n);

			n2 = node_find(ca, &myuser->chanacs);
			node_del(n2, &myuser->chanacs);
			node_free(n2);

			cnt.chanacs--;

			return;
		}
	}
}

void chanacs_delete_host(mychan_t *mychan, char *host, uint32_t level)
{
	chanacs_t *ca;
	node_t *n, *tn;

	if (!mychan || !host)
	{
		slog(LG_DEBUG, "chanacs_delete_host(): got mychan == NULL or myuser == NULL, ignoring");
		return;
	}

	LIST_FOREACH_SAFE(n, tn, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if ((ca->myuser == NULL) && (!irccasecmp(host, ca->host)) && (ca->level == level))
		{
			slog(LG_DEBUG, "chanacs_delete_host(): %s -> %s", ca->mychan->name, ca->host);

			node_del(n, &mychan->chanacs);
			node_free(n);

			BlockHeapFree(chanacs_heap, ca);

			cnt.chanacs--;

			return;
		}
	}
}

chanacs_t *chanacs_find(mychan_t *mychan, myuser_t *myuser, uint32_t level)
{
	node_t *n;
	chanacs_t *ca;

	if ((!mychan) || (!myuser))
		return NULL;

	LIST_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (level != 0x0)
		{
			if ((ca->myuser == myuser) && ((ca->level & level) == level))
				return ca;
		}
		else if (ca->myuser == myuser)
			return ca;
	}

	return NULL;
}

chanacs_t *chanacs_find_host(mychan_t *mychan, char *host, uint32_t level)
{
	node_t *n;
	chanacs_t *ca;

	if ((!mychan) || (!host))
		return NULL;

	LIST_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (level != 0x0)
		{
			if ((ca->myuser == NULL) && (!match(ca->host, host)) && ((ca->level & level) == level))
				return ca;
		}
		else if ((ca->myuser == NULL) && (!match(ca->host, host)))
			return ca;
	}

	return NULL;
}

uint32_t chanacs_host_flags(mychan_t *mychan, char *host)
{
	node_t *n;
	chanacs_t *ca;
	uint32_t result = 0;

	if ((!mychan) || (!host))
		return 0;

	LIST_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (ca->myuser == NULL && !match(ca->host, host))
			result |= ca->level;
	}

	return result;
}

chanacs_t *chanacs_find_host_literal(mychan_t *mychan, char *host, uint32_t level)
{
	node_t *n;
	chanacs_t *ca;

	if ((!mychan) || (!host))
		return NULL;

	LIST_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (level != 0x0)
		{
			if ((ca->myuser == NULL) && (!strcasecmp(ca->host, host)) && ((ca->level & level) == level))
				return ca;
		}
		else if ((ca->myuser == NULL) && (!strcasecmp(ca->host, host)))
			return ca;
	}

	return NULL;
}

chanacs_t *chanacs_find_host_by_user(mychan_t *mychan, user_t *u, uint32_t level)
{
	char host[BUFSIZE];

	if ((!mychan) || (!u))
		return NULL;

	/* construct buffer for user's host */
	strlcpy(host, u->nick, BUFSIZE);
	strlcat(host, "!", BUFSIZE);
	strlcat(host, u->user, BUFSIZE);
	strlcat(host, "@", BUFSIZE);
	strlcat(host, u->vhost, BUFSIZE);

	return chanacs_find_host(mychan, host, level);
}

uint32_t chanacs_host_flags_by_user(mychan_t *mychan, user_t *u)
{
	char host[BUFSIZE];

	if ((!mychan) || (!u))
		return 0;

	/* construct buffer for user's host */
	strlcpy(host, u->nick, BUFSIZE);
	strlcat(host, "!", BUFSIZE);
	strlcat(host, u->user, BUFSIZE);
	strlcat(host, "@", BUFSIZE);
	strlcat(host, u->vhost, BUFSIZE);

	return chanacs_host_flags(mychan, host);
}

chanacs_t *chanacs_find_by_mask(mychan_t *mychan, char *mask, uint32_t level)
{
	myuser_t *mu = myuser_find(mask);

	if (!mychan || !mask)
		return NULL;

	if (mu)
	{
		chanacs_t *ca = chanacs_find(mychan, mu, level);

		if (ca)
			return ca;
	}

	return chanacs_find_host_literal(mychan, mask, level);
}

boolean_t chanacs_user_has_flag(mychan_t *mychan, user_t *u, uint32_t level)
{
	myuser_t *mu;

	if (!mychan || !u)
		return FALSE;

	mu = u->myuser;
	if (mu != NULL)
	{
		if (chanacs_find(mychan, mu, level))
			return TRUE;
	}

	if (chanacs_find_host_by_user(mychan, u, level))
		return TRUE;

	return FALSE;
}

uint32_t chanacs_user_flags(mychan_t *mychan, user_t *u)
{
	myuser_t *mu;
	chanacs_t *ca;
	uint32_t result = 0;

	if (!mychan || !u)
		return FALSE;

	mu = u->myuser;
	if (mu != NULL)
	{
		ca = chanacs_find(mychan, mu, 0);
		if (ca != NULL)
			result |= ca->level;
	}

	result |= chanacs_host_flags_by_user(mychan, u);

	return result;
}

boolean_t chanacs_source_has_flag(mychan_t *mychan, sourceinfo_t *si, uint32_t level)
{
	return si->su != NULL ? chanacs_user_has_flag(mychan, si->su, level) :
		chanacs_find(mychan, si->smu, level) != NULL;
}

uint32_t chanacs_source_flags(mychan_t *mychan, sourceinfo_t *si)
{
	chanacs_t *ca;

	if (si->su != NULL)
	{
		return chanacs_user_flags(mychan, si->su);
	}
	else
	{
		ca = chanacs_find(mychan, si->smu, 0);
		return ca != NULL ? ca->level : 0;
	}
}

/* Change channel access
 *
 * Either mu or hostmask must be specified.
 * Add the flags in *addflags and remove the flags in *removeflags, updating
 * these to reflect the actual change. Only allow changes to restrictflags.
 * Returns true if successful, false if an unallowed change was attempted.
 * -- jilles */
boolean_t chanacs_change(mychan_t *mychan, myuser_t *mu, char *hostmask, uint32_t *addflags, uint32_t *removeflags, uint32_t restrictflags)
{
	chanacs_t *ca;

	if (mychan == NULL)
		return FALSE;
	if (mu == NULL && hostmask == NULL)
	{
		slog(LG_DEBUG, "chanacs_change(): [%s] mu and hostmask both NULL", mychan->name);
		return FALSE;
	}
	if (mu != NULL && hostmask != NULL)
	{
		slog(LG_DEBUG, "chanacs_change(): [%s] mu and hostmask both not NULL", mychan->name);
		return FALSE;
	}
	if (mu != NULL)
	{
		ca = chanacs_find(mychan, mu, 0);
		if (ca == NULL)
		{
			*removeflags = 0;
			/* no change? */
			if ((*addflags | *removeflags) == 0)
				return TRUE;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return FALSE;
			chanacs_add(mychan, mu, *addflags);
		}
		else
		{
			*addflags &= ~ca->level;
			*removeflags &= ca->level & ~*addflags;
			/* no change? */
			if ((*addflags | *removeflags) == 0)
				return TRUE;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return FALSE;
			/* attempting to remove bad flag? */
			if (~restrictflags & *removeflags)
				return FALSE;
			/* attempting to manipulate user with more privs? */
			if (~restrictflags & ca->level)
				return FALSE;
			ca->level = (ca->level | *addflags) & ~*removeflags;
			if (ca->level == 0)
				chanacs_delete(mychan, mu, ca->level);
		}
	}
	else /* hostmask != NULL */
	{
		ca = chanacs_find_host_literal(mychan, hostmask, 0);
		if (ca == NULL)
		{
			*removeflags = 0;
			/* no change? */
			if ((*addflags | *removeflags) == 0)
				return TRUE;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return FALSE;
			chanacs_add_host(mychan, hostmask, *addflags);
		}
		else
		{
			*addflags &= ~ca->level;
			*removeflags &= ca->level & ~*addflags;
			/* no change? */
			if ((*addflags | *removeflags) == 0)
				return TRUE;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return FALSE;
			/* attempting to remove bad flag? */
			if (~restrictflags & *removeflags)
				return FALSE;
			/* attempting to manipulate user with more privs? */
			if (~restrictflags & ca->level)
				return FALSE;
			ca->level = (ca->level | *addflags) & ~*removeflags;
			if (ca->level == 0)
				chanacs_delete_host(mychan, hostmask, ca->level);
		}
	}
	return TRUE;
}

/* version that doesn't return the changes made */
boolean_t chanacs_change_simple(mychan_t *mychan, myuser_t *mu, char *hostmask, uint32_t addflags, uint32_t removeflags, uint32_t restrictflags)
{
	uint32_t a, r;

	a = addflags;
	r = removeflags;
	return chanacs_change(mychan, mu, hostmask, &a, &r, restrictflags);
}

/*******************
 * M E T A D A T A *
 *******************/

metadata_t *metadata_add(void *target, int32_t type, const char *name, const char *value)
{
	myuser_t *mu = NULL;
	mychan_t *mc = NULL;
	chanacs_t *ca = NULL;
	metadata_t *md;
	node_t *n;

	if (!name || !value)
		return NULL;

	if (type == METADATA_USER)
		mu = target;
	else if (type == METADATA_CHANNEL)
		mc = target;
	else if (type == METADATA_CHANACS)
		ca = target;
	else
	{
		slog(LG_DEBUG, "metadata_add(): called on unknown type %d", type);
		return NULL;
	}

	if ((md = metadata_find(target, type, name)))
		metadata_delete(target, type, name);

	md = BlockHeapAlloc(metadata_heap);

	md->name = sstrdup(name);
	md->value = sstrdup(value);

	n = node_create();

	if (type == METADATA_USER)
		node_add(md, n, &mu->metadata);
	else if (type == METADATA_CHANNEL)
		node_add(md, n, &mc->metadata);
	else if (type == METADATA_CHANACS)
		node_add(md, n, &ca->metadata);
	else
	{
		slog(LG_DEBUG, "metadata_add(): trying to add metadata to unknown type %d", type);

		free(md->name);
		free(md->value);
		BlockHeapFree(metadata_heap, md);

		return NULL;
	}

	if (!strncmp("private:", md->name, 8))
		md->private = TRUE;

	return md;
}

void metadata_delete(void *target, int32_t type, const char *name)
{
	node_t *n;
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	metadata_t *md = metadata_find(target, type, name);

	if (!md)
		return;

	if (type == METADATA_USER)
	{
		mu = target;
		n = node_find(md, &mu->metadata);
		node_del(n, &mu->metadata);
		node_free(n);
	}
	else if (type == METADATA_CHANNEL)
	{
		mc = target;
		n = node_find(md, &mc->metadata);
		node_del(n, &mc->metadata);
		node_free(n);
	}
	else if (type == METADATA_CHANACS)
	{
		ca = target;
		n = node_find(md, &ca->metadata);
		node_del(n, &ca->metadata);
		node_free(n);
	}
	else
	{
		slog(LG_DEBUG, "metadata_delete(): trying to delete metadata from unknown type %d", type);
		return;
	}

	free(md->name);
	free(md->value);

	BlockHeapFree(metadata_heap, md);
}

metadata_t *metadata_find(void *target, int32_t type, const char *name)
{
	node_t *n;
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	list_t *l = NULL;
	metadata_t *md;

	if (!name)
		return NULL;

	if (type == METADATA_USER)
	{
		mu = target;
		l = &mu->metadata;
	}
	else if (type == METADATA_CHANNEL)
	{
		mc = target;
		l = &mc->metadata;
	}
	else if (type == METADATA_CHANACS)
	{
		ca = target;
		l = &ca->metadata;
	}
	else
	{
		slog(LG_DEBUG, "metadata_find(): trying to lookup metadata on unknown type %d", type);
		return NULL;
	}

	LIST_FOREACH(n, l->head)
	{
		md = n->data;

		if (!strcasecmp(md->name, name))
			return md;
	}

	return NULL;
}

static int expire_myuser_cb(dictionary_elem_t *delem, void *unused)
{
	myuser_t *mu = (myuser_t *) delem->node.data;

	/* If they're logged in, update lastlogin time.
	 * To decrease db traffic, may want to only do
	 * this if the account would otherwise be
	 * deleted. -- jilles 
	 */
	if (LIST_LENGTH(&mu->logins) > 0)
	{
		mu->lastlogin = CURRTIME;
		return 0;
	}

	if (MU_HOLD & mu->flags)
		return 0;

	if (((CURRTIME - mu->lastlogin) >= config_options.expire) || ((mu->flags & MU_WAITAUTH) && (CURRTIME - mu->registered >= 86400)))
	{
		/* Don't expire accounts with privs on them in atheme.conf,
		 * otherwise someone can reregister
		 * them and take the privs -- jilles */
		if (is_conf_soper(mu))
			return 0;

		snoop("EXPIRE: \2%s\2 from \2%s\2 ", mu->name, mu->email);
		myuser_delete(mu);
	}

	return 0;
}

void expire_check(void *arg)
{
	mynick_t *mn;
	mychan_t *mc;
	user_t *u;
	dictionary_iteration_state_t state;

	/* Let them know about this and the likely subsequent db_save()
	 * right away -- jilles */
	sendq_flush(curr_uplink->conn);

	if (config_options.expire == 0)
		return;

	dictionary_foreach(mulist, expire_myuser_cb, NULL);

	DICTIONARY_FOREACH(mn, &state, nicklist)
	{
		if ((CURRTIME - mn->lastseen) >= config_options.expire)
		{
			if (MU_HOLD & mn->owner->flags)
				continue;

			/* do not drop main nick like this */
			if (!irccasecmp(mn->nick, mn->owner->name))
				continue;

			u = user_find_named(mn->nick);
			if (u != NULL && u->myuser == mn->owner)
			{
				/* still logged in, bleh */
				mn->lastseen = CURRTIME;
				mn->owner->lastlogin = CURRTIME;
				continue;
			}

			snoop("EXPIRE: \2%s\2 from \2%s\2", mn->nick, mn->owner->name);
			mynick_delete(mn);
		}
	}

	DICTIONARY_FOREACH(mc, &state, mclist)
	{
		if ((CURRTIME - mc->used) >= 86400 - 3660)
		{
			/* keep last used time accurate to
			 * within a day, making sure an active
			 * channel will never get "Last used"
			 * in /cs info -- jilles */
			if (mychan_isused(mc))
			{
				mc->used = CURRTIME;
				slog(LG_DEBUG, "expire_check(): updating last used time on %s because it appears to be still in use", mc->name);
				continue;
			}
		}

		if ((CURRTIME - mc->used) >= config_options.expire)
		{
			if (MU_HOLD & mc->founder->flags)
				continue;

			if (MC_HOLD & mc->flags)
				continue;

			snoop("EXPIRE: \2%s\2 from \2%s\2", mc->name, mc->founder->name);

			hook_call_event("channel_drop", mc);
			if ((config_options.chan && irccasecmp(mc->name, config_options.chan)) || !config_options.chan)
				part(mc->name, chansvs.nick);

			mychan_delete(mc->name);
		}
	}
}

static int check_myuser_cb(dictionary_elem_t *delem, void *unused)
{
	myuser_t *mu = (myuser_t *) delem->node.data;
	mynick_t *mn;

	if (MU_OLD_ALIAS & mu->flags)
	{
		slog(LG_INFO, "db_check(): converting previously linked nick %s to a standalone nick", mu->name);
		mu->flags &= ~MU_OLD_ALIAS;
		metadata_delete(mu, METADATA_USER, "private:alias:parent");
	}

	if (!nicksvs.no_nick_ownership)
	{
		mn = mynick_find(mu->name);
		if (mn == NULL)
		{
			slog(LG_DEBUG, "db_check(): adding missing nick %s", mu->name);
			mn = mynick_add(mu, mu->name);
			mn->registered = mu->registered;
			mn->lastseen = mu->lastlogin;
		}
		else if (mn->owner != mu)
		{
			slog(LG_INFO, "db_check(): replacing nick %s owned by %s with %s", mn->nick, mn->owner->name, mu->name);
			mynick_delete(mn);
			mn = mynick_add(mu, mu->name);
			mn->registered = mu->registered;
			mn->lastseen = mu->lastlogin;
		}
	}

	return 0;
}

void db_check()
{
	mychan_t *mc;
	dictionary_iteration_state_t state;

	dictionary_foreach(mulist, check_myuser_cb, NULL);

	DICTIONARY_FOREACH(mc, &state, mclist)
	{
		if (!chanacs_find(mc, mc->founder, CA_FLAGS))
		{
			slog(LG_INFO, "db_check(): adding access for founder on channel %s", mc->name);
			chanacs_change_simple(mc, mc->founder, NULL, CA_FOUNDER_0, 0, CA_ALL);
		}
	}
}
