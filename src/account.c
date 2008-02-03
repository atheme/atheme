/*
 * atheme-services: A collection of minimalist IRC services
 * account.c: Account management
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"
#include "uplink.h" /* XXX, for sendq_flush(curr_uplink->conn); */
#include "datastream.h"
#include "privs.h"
#include "authcookie.h"

mowgli_patricia_t *mulist;
mowgli_patricia_t *nicklist;
mowgli_patricia_t *oldnameslist;
mowgli_patricia_t *mclist;

static BlockHeap *myuser_heap;  /* HEAP_USER */
static BlockHeap *mynick_heap;  /* HEAP_USER */
static BlockHeap *myuser_name_heap;	/* HEAP_USER / 2 */
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
	myuser_name_heap = BlockHeapCreate(sizeof(myuser_name_t), HEAP_USER / 2);
	mychan_heap = BlockHeapCreate(sizeof(mychan_t), HEAP_CHANNEL);
	chanacs_heap = BlockHeapCreate(sizeof(chanacs_t), HEAP_CHANUSER);
	metadata_heap = BlockHeapCreate(sizeof(metadata_t), HEAP_CHANUSER);

	if (myuser_heap == NULL || mynick_heap == NULL || mychan_heap == NULL
			|| chanacs_heap == NULL || metadata_heap == NULL)
	{
		slog(LG_ERROR, "init_accounts(): block allocator failure.");
		exit(EXIT_FAILURE);
	}

	mulist = mowgli_patricia_create(irccasecanon);
	nicklist = mowgli_patricia_create(irccasecanon);
	oldnameslist = mowgli_patricia_create(irccasecanon);
	mclist = mowgli_patricia_create(irccasecanon);
}

/*
 * myuser_add(char *name, char *pass, char *email, unsigned int flags)
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
myuser_t *myuser_add(char *name, char *pass, char *email, unsigned int flags)
{
	myuser_t *mu;
	soper_t *soper;

	return_val_if_fail((mu = myuser_find(name)) == NULL, mu);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "myuser_add(): %s -> %s", name, email);

	mu = BlockHeapAlloc(myuser_heap);
	object_init(object(mu), NULL, (destructor_t) myuser_delete);

	strlcpy(mu->name, name, NICKLEN);
	strlcpy(mu->email, email, EMAILLEN);

	mu->registered = CURRTIME;
	mu->flags = flags;
	if (mu->flags & MU_ENFORCE)
	{
		mu->flags &= ~MU_ENFORCE;
		metadata_add(mu, METADATA_USER, "private:doenforce", "1");
	}

	/* If it's already crypted, don't touch the password. Otherwise,
	 * use set_password() to initialize it. Why? Because set_password
	 * will move the user to encrypted passwords if possible. That way,
	 * new registers are immediately protected and the database is
	 * immediately converted the first time we start up with crypto.
	 */
	if (flags & MU_CRYPTPASS)
		strlcpy(mu->pass, pass, PASSLEN);
	else
		set_password(mu, pass);

	mowgli_patricia_add(mulist, mu->name, mu);

	if ((soper = soper_find_named(mu->name)) != NULL)
	{
		slog(LG_DEBUG, "myuser_add(): user `%s' has been declared as soper, activating privileges.", mu->name);
		soper->myuser = mu;
		mu->soper = soper;
	}

	myuser_name_restore(mu->name, mu);

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
	user_t *u;
	node_t *n, *tn;
	metadata_t *md;
	mymemo_t *memo;
	chanacs_t *ca;

	return_if_fail(mu != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "myuser_delete(): %s", mu->name);

	myuser_name_remember(mu->name, mu);

	/* log them out */
	LIST_FOREACH_SAFE(n, tn, mu->logins.head)
	{
		u = (user_t *)n->data;
		if (!authservice_loaded || !ircd_on_logout(u->nick, mu->name, NULL))
		{
			u->myuser = NULL;
			node_del(n, &mu->logins);
			node_free(n);
		}
	}

	/* kill all their channels and chanacs */
	LIST_FOREACH_SAFE(n, tn, mu->chanacs.head)
	{
		ca = n->data;
		mc = ca->mychan;

		/* attempt succession */
		if (ca->level & CA_FOUNDER && mychan_num_founders(mc) == 1 && (successor = mychan_pick_successor(mc)) != NULL)
		{
			snoop(_("SUCCESSION: \2%s\2 -> \2%s\2 from \2%s\2"), successor->name, mc->name, mu->name);
			slog(LG_REGISTER, "myuser_delete(): giving channel %s to %s (unused %ds, founder %s, chanacs %d)",
					mc->name, successor->name,
					CURRTIME - mc->used,
					mu->name,
					LIST_LENGTH(&mc->chanacs));
			if (chansvs.me != NULL)
				verbose(mc, "Foundership changed to \2%s\2 because \2%s\2 was dropped.", successor->name, mu->name);

			chanacs_change_simple(mc, successor, NULL, CA_FOUNDER_0, 0);
			if (chansvs.me != NULL)
				myuser_notice(chansvs.nick, successor, "You are now founder on \2%s\2 (as \2%s\2).", mc->name, successor->name);
			object_unref(ca);
		}
		/* no successor found */
		else if (ca->level & CA_FOUNDER && mychan_num_founders(mc) == 1)
		{
			snoop(_("DELETE: \2%s\2 from \2%s\2"), mc->name, mu->name);
			slog(LG_REGISTER, "myuser_delete(): deleting channel %s (unused %ds, founder %s, chanacs %d)",
					mc->name, CURRTIME - mc->used,
					mu->name,
					LIST_LENGTH(&mc->chanacs));

			hook_call_event("channel_drop", mc);
			if ((config_options.chan && irccasecmp(mc->name, config_options.chan)) || !config_options.chan)
				part(mc->name, chansvs.nick);
			object_unref(mc);
		}
		else /* not founder */
			object_unref(ca);
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

	/* delete memos */
	LIST_FOREACH_SAFE(n, tn, mu->memos.head)
	{
		memo = (mymemo_t *)n->data;

		node_del(n, &mu->memos);
		node_free(n);
		free(memo);
	}

	/* delete access entries */
	LIST_FOREACH_SAFE(n, tn, mu->access_list.head)
		myuser_access_delete(mu, (char *)n->data);

	/* delete their nicks */
	LIST_FOREACH_SAFE(n, tn, mu->nicks.head)
		object_unref(n->data);

	/* mu->name is the index for this dtree */
	mowgli_patricia_delete(mulist, mu->name);

	BlockHeapFree(myuser_heap, mu);

	cnt.myuser--;
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

int
myuser_num_channels(myuser_t *mu)
{
	node_t *n;
	chanacs_t *ca;
	int count = 0;

	LIST_FOREACH(n, mu->chanacs.head)
	{
		ca = n->data;
		if (ca->level & CA_FOUNDER)
			count++;
	}
	return count;
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
			node_free(n);
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

	return_val_if_fail((mn = mynick_find(name)) == NULL, mn);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "mynick_add(): %s -> %s", name, mu->name);

	mn = BlockHeapAlloc(mynick_heap);
	object_init(object(mn), NULL, (destructor_t) mynick_delete);

	strlcpy(mn->nick, name, NICKLEN);
	mn->owner = mu;
	mn->registered = CURRTIME;

	mowgli_patricia_add(nicklist, mn->nick, mn);
	node_add(mn, &mn->node, &mu->nicks);

	myuser_name_restore(mn->nick, mu);

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
	return_if_fail(mn != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "mynick_delete(): %s", mn->nick);

	myuser_name_remember(mn->nick, mn->owner);

	mowgli_patricia_delete(nicklist, mn->nick);
	node_del(&mn->node, &mn->owner->nicks);

	BlockHeapFree(mynick_heap, mn);

	cnt.mynick--;
}

/*************************
 * M Y U S E R _ N A M E *
 *************************/

static void myuser_name_delete(myuser_name_t *mun);

/*
 * myuser_name_add(const char *name)
 *
 * Creates a record for the given name and adds it to the oldnames DTree.
 *
 * Inputs:
 *      - a nick or account name
 *
 * Outputs:
 *      - on success, a new myuser_name_t object
 *      - on failure, NULL.
 *
 * Side Effects:
 *      - the created record is added to the oldnames DTree.
 */
myuser_name_t *myuser_name_add(const char *name)
{
	myuser_name_t *mun;

	return_val_if_fail((mun = myuser_name_find(name)) == NULL, mun);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "myuser_name_add(): %s", name);

	mun = BlockHeapAlloc(myuser_name_heap);
	object_init(object(mun), NULL, (destructor_t) myuser_name_delete);

	strlcpy(mun->name, name, NICKLEN);

	mowgli_patricia_add(oldnameslist, mun->name, mun);

	cnt.myuser_name++;

	return mun;
}

/*
 * myuser_name_delete(myuser_name_t *mun)
 *
 * Destroys and removes a name from the oldnames DTree.
 *
 * Inputs:
 *      - record to destroy
 *
 * Outputs:
 *      - nothing
 *
 * Side Effects:
 *      - a record is destroyed and removed from the oldnames DTree.
 */
static void myuser_name_delete(myuser_name_t *mun)
{
	metadata_t *md;
	node_t *n, *tn;

	return_if_fail(mun != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "myuser_name_delete(): %s", mun->name);

	mowgli_patricia_delete(oldnameslist, mun->name);

	/* delete the metadata */
	LIST_FOREACH_SAFE(n, tn, mun->metadata.head)
	{
		md = n->data;
		metadata_delete(mun, METADATA_USER_NAME, md->name);
	}

	BlockHeapFree(myuser_name_heap, mun);

	cnt.myuser_name--;
}

/*
 * myuser_name_remember(const char *name, myuser_t *mu)
 *
 * If the given account has any information worth saving, creates a record
 * for the given name and adds it to the oldnames DTree.
 *
 * Inputs:
 *      - a nick or account name
 *      - an account pointer
 *
 * Outputs:
 *      - none
 *
 * Side Effects:
 *      - a record may be added to the oldnames DTree.
 */
void myuser_name_remember(const char *name, myuser_t *mu)
{
	myuser_name_t *mun;
	metadata_t *md;

	if (myuser_name_find(name))
		return;

	md = metadata_find(mu, METADATA_USER, "private:mark:setter");
	if (md == NULL)
		return;

	mun = myuser_name_add(name);

	metadata_add(mun, METADATA_USER_NAME, md->name, md->value);
	md = metadata_find(mu, METADATA_USER, "private:mark:reason");
	if (md != NULL)
		metadata_add(mun, METADATA_USER_NAME, md->name, md->value);
	md = metadata_find(mu, METADATA_USER, "private:mark:timestamp");
	if (md != NULL)
		metadata_add(mun, METADATA_USER_NAME, md->name, md->value);

	return;
}

/*
 * myuser_name_restore(const char *name, myuser_t *mu)
 *
 * If the given name is in the oldnames DTree, restores information from it
 * into the given account.
 *
 * Inputs:
 *      - a nick or account name
 *      - an account pointer
 *
 * Outputs:
 *      - none
 *
 * Side Effects:
 *      - if present, the record will be removed from the oldnames DTree.
 */
void myuser_name_restore(const char *name, myuser_t *mu)
{
	myuser_name_t *mun;
	metadata_t *md, *md2;
	node_t *n;

	mun = myuser_name_find(name);
	if (mun == NULL)
		return;

	md = metadata_find(mu, METADATA_USER, "private:mark:reason");
	md2 = metadata_find(mun, METADATA_USER_NAME, "private:mark:reason");
	if (md != NULL && md2 != NULL && strcmp(md->value, md2->value))
	{
		wallops(_("Not restoring mark \2\"%s\"\2 for account \2%s\2 (name \2%s\2) which is already marked"), md2->value, mu->name, name);
		snoop(_("MARK:FORGET: \2\"%s\"\2 for \2%s (%s)\2 (already marked)"), md2->value, name, mu->name);
		slog(LG_INFO, "myuser_name_restore(): not restoring mark \"%s\" for account %s (name %s) which is already marked",
				md2->value, mu->name, name);
	}
	else if (md == NULL && md2 != NULL)
	{
		snoop(_("MARK:RESTORE: \2\"%s\"\2 for \2%s (%s)\2"), md2->value, name, mu->name);
		slog(LG_INFO, "myuser_name_restore(): restoring mark \"%s\" for account %s (name %s)",
				md2->value, mu->name, name);
	}

	LIST_FOREACH(n, mun->metadata.head)
	{
		md = n->data;
		/* prefer current metadata to saved */
		if (!metadata_find(mu, METADATA_USER, md->name))
			metadata_add(mu, METADATA_USER, md->name, md->value);
	}

	object_unref(mun);

	return;
}

/***************
 * M Y C H A N *
 ***************/

/* private destructor for mychan_t. */
static void mychan_delete(mychan_t *mc)
{
	metadata_t *md;
	node_t *n, *tn;

	return_if_fail(mc != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "mychan_delete(): %s", mc->name);

	/* remove the chanacs shiz */
	LIST_FOREACH_SAFE(n, tn, mc->chanacs.head)
		object_unref(n->data);

	/* delete the metadata */
	LIST_FOREACH_SAFE(n, tn, mc->metadata.head)
	{
		md = (metadata_t *) n->data;
		metadata_delete(mc, METADATA_CHANNEL, md->name);
	}

	mowgli_patricia_delete(mclist, mc->name);

	BlockHeapFree(mychan_heap, mc);

	cnt.mychan--;
}

mychan_t *mychan_add(char *name)
{
	mychan_t *mc;

	return_val_if_fail((mc = mychan_find(name)) == NULL, mc);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "mychan_add(): %s", name);

	mc = BlockHeapAlloc(mychan_heap);

	object_init(object(mc), NULL, (destructor_t) mychan_delete);
	strlcpy(mc->name, name, CHANNELLEN);
	mc->registered = CURRTIME;
	mc->chan = channel_find(name);

	mowgli_patricia_add(mclist, mc->name, mc);

	cnt.mychan++;

	return mc;
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

int mychan_num_founders(mychan_t *mc)
{
	node_t *n;
	chanacs_t *ca;
	int count = 0;

	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;
		if (ca->myuser != NULL && ca->level & CA_FOUNDER)
			count++;
	}
	return count;
}

const char *mychan_founder_names(mychan_t *mc)
{
	node_t *n;
	chanacs_t *ca;
	static char names[512];

	names[0] = '\0';
	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;
		if (ca->myuser != NULL && ca->level & CA_FOUNDER)
		{
			if (names[0] != '\0')
				strlcat(names, ", ", sizeof names);
			strlcat(names, ca->myuser->name, sizeof names);
		}
	}
	return names;
}

/* Find a user fulfilling the conditions who can take another channel */
myuser_t *mychan_pick_candidate(mychan_t *mc, unsigned int minlevel, int maxtime)
{
	node_t *n;
	chanacs_t *ca;
	myuser_t *mu;

	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;
		if (ca->level & CA_AKICK)
			continue;
		mu = ca->myuser;
		if (mu == NULL || ca->level & CA_FOUNDER)
			continue;
		if ((ca->level & minlevel) == minlevel && (maxtime == 0 || LIST_LENGTH(&mu->logins) > 0 || CURRTIME - mu->lastlogin < maxtime))
		{
			if (has_priv_myuser(mu, PRIV_REG_NOLIMIT))
				return mu;
			if (myuser_num_channels(mu) < me.maxchans)
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
	mu = mychan_pick_candidate(mc, CA_FOUNDER_0 & ca_all & ~CA_FOUNDER, 7*86400);
	if (mu != NULL)
		return mu;
	mu = mychan_pick_candidate(mc, CA_FOUNDER_0 & ca_all & ~CA_FOUNDER, 0);
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

/* private destructor for chanacs_t */
static void chanacs_delete(chanacs_t *ca)
{
	node_t *n, *tn;

	return_if_fail(ca != NULL);
	return_if_fail(ca->mychan != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "chanacs_delete(): %s -> %s", ca->mychan->name,
			ca->myuser != NULL ? ca->myuser->name : ca->host);
	n = node_find(ca, &ca->mychan->chanacs);
	node_del(n, &ca->mychan->chanacs);
	node_free(n);

	if (ca->myuser != NULL)
	{
		n = node_find(ca, &ca->myuser->chanacs);
		node_del(n, &ca->myuser->chanacs);
		node_free(n);
	}

	LIST_FOREACH_SAFE(n, tn, ca->metadata.head)
	{
		metadata_t *md = n->data;
		metadata_delete(ca, METADATA_CHANACS, md->name);
	}

	BlockHeapFree(chanacs_heap, ca);

	cnt.chanacs--;
}

/*
 * chanacs_add(mychan_t *mychan, myuser_t *myuser, unsigned int level, time_t ts)
 *
 * Creates an access entry mapping between a user and channel.
 *
 * Inputs:
 *       - a channel to create a mapping for
 *       - a user to create a mapping for
 *       - a bitmask which describes the access entry's privileges
 *       - a timestamp for this access entry
 *
 * Outputs:
 *       - a chanacs_t object which describes the mapping
 *
 * Side Effects:
 *       - the channel access list is updated for mychan.
 */
chanacs_t *chanacs_add(mychan_t *mychan, myuser_t *myuser, unsigned int level, time_t ts)
{
	chanacs_t *ca;
	node_t *n1;
	node_t *n2;

	return_val_if_fail(mychan != NULL && myuser != NULL, NULL);

	if (*mychan->name != '#')
	{
		slog(LG_DEBUG, "chanacs_add(): got non #channel: %s", mychan->name);
		return NULL;
	}

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "chanacs_add(): %s -> %s", mychan->name, myuser->name);

	n1 = node_create();
	n2 = node_create();

	ca = BlockHeapAlloc(chanacs_heap);

	object_init(object(ca), NULL, (destructor_t) chanacs_delete);
	ca->mychan = mychan;
	ca->myuser = myuser;
	ca->level = level & ca_all;
	ca->ts = ts;

	node_add(ca, n1, &mychan->chanacs);
	node_add(ca, n2, &myuser->chanacs);

	cnt.chanacs++;

	return ca;
}

/*
 * chanacs_add_host(mychan_t *mychan, char *host, unsigned int level, time_t ts)
 *
 * Creates an access entry mapping between a hostmask and channel.
 *
 * Inputs:
 *       - a channel to create a mapping for
 *       - a hostmask to create a mapping for
 *       - a bitmask which describes the access entry's privileges
 *       - a timestamp for this access entry
 *
 * Outputs:
 *       - a chanacs_t object which describes the mapping
 *
 * Side Effects:
 *       - the channel access list is updated for mychan.
 */
chanacs_t *chanacs_add_host(mychan_t *mychan, const char *host, unsigned int level, time_t ts)
{
	chanacs_t *ca;
	node_t *n;

	return_val_if_fail(mychan != NULL && host != NULL, NULL);

	if (*mychan->name != '#')
	{
		slog(LG_DEBUG, "chanacs_add_host(): got non #channel: %s", mychan->name);
		return NULL;
	}

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "chanacs_add_host(): %s -> %s", mychan->name, host);

	n = node_create();

	ca = BlockHeapAlloc(chanacs_heap);

	object_init(object(ca), NULL, (destructor_t) chanacs_delete);
	ca->mychan = mychan;
	ca->myuser = NULL;
	strlcpy(ca->host, host, HOSTLEN);
	ca->level = level & ca_all;
	ca->ts = ts;

	node_add(ca, n, &mychan->chanacs);

	cnt.chanacs++;

	return ca;
}

chanacs_t *chanacs_find(mychan_t *mychan, myuser_t *myuser, unsigned int level)
{
	node_t *n;
	chanacs_t *ca;

	return_val_if_fail(mychan != NULL && myuser != NULL, NULL);

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

chanacs_t *chanacs_find_host(mychan_t *mychan, const char *host, unsigned int level)
{
	node_t *n;
	chanacs_t *ca;

	return_val_if_fail(mychan != NULL && host != NULL, NULL);

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

unsigned int chanacs_host_flags(mychan_t *mychan, const char *host)
{
	node_t *n;
	chanacs_t *ca;
	unsigned int result = 0;

	return_val_if_fail(mychan != NULL && host != NULL, 0);

	LIST_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (ca->myuser == NULL && !match(ca->host, host))
			result |= ca->level;
	}

	return result;
}

chanacs_t *chanacs_find_host_literal(mychan_t *mychan, const char *host, unsigned int level)
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

chanacs_t *chanacs_find_host_by_user(mychan_t *mychan, user_t *u, unsigned int level)
{
	node_t *n;
	chanacs_t *ca;

	return_val_if_fail(mychan != NULL && u != NULL, 0);

	for (n = next_matching_host_chanacs(mychan, u, mychan->chanacs.head); n != NULL; n = next_matching_host_chanacs(mychan, u, n->next))
	{
		ca = n->data;
		if ((ca->level & level) == level)
			return ca;
	}

	return NULL;
}

unsigned int chanacs_host_flags_by_user(mychan_t *mychan, user_t *u)
{
	node_t *n;
	unsigned int result = 0;
	chanacs_t *ca;

	return_val_if_fail(mychan != NULL && u != NULL, 0);

	for (n = next_matching_host_chanacs(mychan, u, mychan->chanacs.head); n != NULL; n = next_matching_host_chanacs(mychan, u, n->next))
	{
		ca = n->data;
		result |= ca->level;
	}

	return result;
}

chanacs_t *chanacs_find_by_mask(mychan_t *mychan, const char *mask, unsigned int level)
{
	myuser_t *mu;
	chanacs_t *ca;

	return_val_if_fail(mychan != NULL && mask != NULL, NULL);

	mu = myuser_find(mask);

	if (mu != NULL)
	{
		ca = chanacs_find(mychan, mu, level);

		if (ca)
			return ca;
	}

	return chanacs_find_host_literal(mychan, mask, level);
}

boolean_t chanacs_user_has_flag(mychan_t *mychan, user_t *u, unsigned int level)
{
	myuser_t *mu;

	return_val_if_fail(mychan != NULL && u != NULL, FALSE);

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

unsigned int chanacs_user_flags(mychan_t *mychan, user_t *u)
{
	myuser_t *mu;
	chanacs_t *ca;
	unsigned int result = 0;

	return_val_if_fail(mychan != NULL && u != NULL, 0);

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

unsigned int chanacs_source_flags(mychan_t *mychan, sourceinfo_t *si)
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

/* Look for the chanacs exactly matching mu or host (exactly one of mu and
 * host must be non-NULL). If not found, and create is TRUE, create a new
 * chanacs with no flags.
 */
chanacs_t *chanacs_open(mychan_t *mychan, myuser_t *mu, const char *hostmask, boolean_t create)
{
	chanacs_t *ca;

	/* wrt the second assert: only one of mu or hostmask can be not-NULL --nenolod */
	return_val_if_fail(mychan != NULL, FALSE);
	return_val_if_fail((mu != NULL && hostmask == NULL) || (mu == NULL && hostmask != NULL), FALSE); 

	if (mu != NULL)
	{
		ca = chanacs_find(mychan, mu, 0);
		if (ca != NULL)
			return ca;
		else if (create)
			return chanacs_add(mychan, mu, 0, CURRTIME);
	}
	else
	{
		ca = chanacs_find_host_literal(mychan, hostmask, 0);
		if (ca != NULL)
			return ca;
		else if (create)
			return chanacs_add_host(mychan, hostmask, 0, CURRTIME);
	}
	return NULL;
}

/* Change channel access
 *
 * Either mu or hostmask must be specified.
 * Add the flags in *addflags and remove the flags in *removeflags, updating
 * these to reflect the actual change. Only allow changes to restrictflags.
 * Returns true if successful, false if an unallowed change was attempted.
 * -- jilles */
boolean_t chanacs_modify(chanacs_t *ca, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags)
{
	return_val_if_fail(ca != NULL, FALSE);
	return_val_if_fail(addflags != NULL && removeflags != NULL, FALSE);

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
	ca->ts = CURRTIME;

	return TRUE;
}

/* version that doesn't return the changes made */
boolean_t chanacs_modify_simple(chanacs_t *ca, unsigned int addflags, unsigned int removeflags)
{
	unsigned int a, r;

	a = addflags & ca_all;
	r = removeflags & ca_all;
	return chanacs_modify(ca, &a, &r, ca_all);
}

/* Change channel access
 *
 * Either mu or hostmask must be specified.
 * Add the flags in *addflags and remove the flags in *removeflags, updating
 * these to reflect the actual change. Only allow changes to restrictflags.
 * Returns true if successful, false if an unallowed change was attempted.
 * -- jilles */
boolean_t chanacs_change(mychan_t *mychan, myuser_t *mu, const char *hostmask, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags)
{
	chanacs_t *ca;

	/* wrt the second assert: only one of mu or hostmask can be not-NULL --nenolod */
	return_val_if_fail(mychan != NULL, FALSE);
	return_val_if_fail((mu != NULL && hostmask == NULL) || (mu == NULL && hostmask != NULL), FALSE); 
	return_val_if_fail(addflags != NULL && removeflags != NULL, FALSE);

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
			chanacs_add(mychan, mu, *addflags, CURRTIME);
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
			ca->ts = CURRTIME;
			if (ca->level == 0)
				object_unref(ca);
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
			chanacs_add_host(mychan, hostmask, *addflags, CURRTIME);
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
			ca->ts = CURRTIME;
			if (ca->level == 0)
				object_unref(ca);
		}
	}
	return TRUE;
}

/* version that doesn't return the changes made */
boolean_t chanacs_change_simple(mychan_t *mychan, myuser_t *mu, const char *hostmask, unsigned int addflags, unsigned int removeflags)
{
	unsigned int a, r;

	a = addflags & ca_all;
	r = removeflags & ca_all;
	return chanacs_change(mychan, mu, hostmask, &a, &r, ca_all);
}

/*******************
 * M E T A D A T A *
 *******************/

metadata_t *metadata_add(void *target, int type, const char *name, const char *value)
{
	myuser_t *mu = NULL;
	mychan_t *mc = NULL;
	chanacs_t *ca = NULL;
	myuser_name_t *mun = NULL;
	metadata_t *md;
	hook_metadata_change_t mdchange;

	if (!name || !value)
		return NULL;

	if (type == METADATA_USER)
		mu = target;
	else if (type == METADATA_CHANNEL)
		mc = target;
	else if (type == METADATA_CHANACS)
		ca = target;
	else if (type == METADATA_USER_NAME)
		mun = target;
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

	if (type == METADATA_USER)
		node_add(md, &md->node, &mu->metadata);
	else if (type == METADATA_CHANNEL)
		node_add(md, &md->node, &mc->metadata);
	else if (type == METADATA_CHANACS)
		node_add(md, &md->node, &ca->metadata);
	else if (type == METADATA_USER_NAME)
		node_add(md, &md->node, &mun->metadata);
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

	mdchange.target = target;
	mdchange.name = md->name;
	mdchange.value = md->value;
	mdchange.type = type;
	hook_call_event("metadata_change", &mdchange);

	return md;
}

void metadata_delete(void *target, int type, const char *name)
{
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	myuser_name_t *mun;
	metadata_t *md = metadata_find(target, type, name);
	hook_metadata_change_t mdchange;

	if (!md)
		return;

	mdchange.target = target;
	mdchange.name = name;
	mdchange.value = NULL;
	mdchange.type = type;
	hook_call_event("metadata_change", &mdchange);

	if (type == METADATA_USER)
	{
		mu = target;
		node_del(&md->node, &mu->metadata);
	}
	else if (type == METADATA_CHANNEL)
	{
		mc = target;
		node_del(&md->node, &mc->metadata);
	}
	else if (type == METADATA_CHANACS)
	{
		ca = target;
		node_del(&md->node, &ca->metadata);
	}
	else if (type == METADATA_USER_NAME)
	{
		mun = target;
		node_del(&md->node, &mun->metadata);
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

metadata_t *metadata_find(void *target, int type, const char *name)
{
	node_t *n;
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	myuser_name_t *mun;
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
	else if (type == METADATA_USER_NAME)
	{
		mun = target;
		l = &mun->metadata;
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

static int expire_myuser_cb(const char *key, void *data, void *unused)
{
	myuser_t *mu = (myuser_t *) data;

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

	if ((nicksvs.expiry > 0 && CURRTIME - mu->lastlogin >= nicksvs.expiry) ||
			(mu->flags & MU_WAITAUTH && CURRTIME - mu->registered >= 86400))
	{
		/* Don't expire accounts with privs on them in atheme.conf,
		 * otherwise someone can reregister
		 * them and take the privs -- jilles */
		if (is_conf_soper(mu))
			return 0;

		snoop(_("EXPIRE: \2%s\2 from \2%s\2 "), mu->name, mu->email);
		slog(LG_REGISTER, "expire_check(): expiring account %s (unused %ds, email %s, nicks %d, chanacs %d)",
				mu->name, (int)(CURRTIME - mu->lastlogin),
				mu->email, LIST_LENGTH(&mu->nicks),
				LIST_LENGTH(&mu->chanacs));
		object_unref(mu);
	}

	return 0;
}

void expire_check(void *arg)
{
	mynick_t *mn;
	mychan_t *mc;
	user_t *u;
	mowgli_patricia_iteration_state_t state;

	/* Let them know about this and the likely subsequent db_save()
	 * right away -- jilles */
	sendq_flush(curr_uplink->conn);

	if (nicksvs.expiry >= 0)
	{
		mowgli_patricia_foreach(mulist, expire_myuser_cb, NULL);

		MOWGLI_PATRICIA_FOREACH(mn, &state, nicklist)
		{
			if (nicksvs.expiry > 0 &&
					(CURRTIME - mn->lastseen) >= nicksvs.expiry)
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

				snoop(_("EXPIRE: \2%s\2 from \2%s\2"), mn->nick, mn->owner->name);
				slog(LG_REGISTER, "expire_check(): expiring nick %s (unused %ds, account %s)",
						mn->nick, CURRTIME - mn->lastseen,
						mn->owner->name);
				object_unref(mn);
			}
		}
	}

	if (chansvs.expiry >= 0)
	{
		MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
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

			if (chansvs.expiry > 0 &&
					(CURRTIME - mc->used) >= chansvs.expiry)
			{
				if (MC_HOLD & mc->flags)
					continue;

				snoop(_("EXPIRE: \2%s\2 from \2%s\2"), mc->name, mychan_founder_names(mc));
				slog(LG_REGISTER, "expire_check(): expiring channel %s (unused %ds, founder %s, chanacs %d)",
						mc->name, CURRTIME - mc->used,
						mychan_founder_names(mc),
						LIST_LENGTH(&mc->chanacs));

				hook_call_event("channel_drop", mc);
				if ((config_options.chan && irccasecmp(mc->name, config_options.chan)) || !config_options.chan)
					part(mc->name, chansvs.nick);

				object_unref(mc);
			}
		}
	}
}

static int check_myuser_cb(const char *key, void *data, void *unused)
{
	myuser_t *mu = (myuser_t *) data;
	mynick_t *mn;

	if (MU_OLD_ALIAS & mu->flags)
	{
		slog(LG_REGISTER, "db_check(): converting previously linked nick %s to a standalone nick", mu->name);
		mu->flags &= ~MU_OLD_ALIAS;
		metadata_delete(mu, METADATA_USER, "private:alias:parent");
	}

	if (!nicksvs.no_nick_ownership)
	{
		mn = mynick_find(mu->name);
		if (mn == NULL)
		{
			slog(LG_REGISTER, "db_check(): adding missing nick %s", mu->name);
			mn = mynick_add(mu, mu->name);
			mn->registered = mu->registered;
			mn->lastseen = mu->lastlogin;
		}
		else if (mn->owner != mu)
		{
			slog(LG_REGISTER, "db_check(): replacing nick %s owned by %s with %s", mn->nick, mn->owner->name, mu->name);
			object_unref(mn);
			mn = mynick_add(mu, mu->name);
			mn->registered = mu->registered;
			mn->lastseen = mu->lastlogin;
		}
	}

	return 0;
}

void db_check(void)
{
	mowgli_patricia_foreach(mulist, check_myuser_cb, NULL);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
