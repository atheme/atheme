/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2015 Atheme Project (http://atheme.org/)
 * Copyright (C) 2015-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * account.c: Account management
 */

#include <atheme.h>
#include "internal.h"

mowgli_patricia_t *nicklist;
mowgli_patricia_t *oldnameslist;
mowgli_patricia_t *mclist;

static mowgli_patricia_t *certfplist;

static mowgli_heap_t *myuser_heap;   /* HEAP_USER */
static mowgli_heap_t *mynick_heap;   /* HEAP_USER */
static mowgli_heap_t *mycertfp_heap; /* HEAP_USER */
static mowgli_heap_t *myuser_name_heap;	/* HEAP_USER / 2 */
static mowgli_heap_t *mychan_heap;	/* HEAP_CHANNEL */
static mowgli_heap_t *chanacs_heap;	/* HEAP_CHANACS */

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
void
init_accounts(void)
{
	myuser_heap = sharedheap_get(sizeof(struct myuser));
	mynick_heap = sharedheap_get(sizeof(struct myuser));
	myuser_name_heap = sharedheap_get(sizeof(struct myuser_name));
	mychan_heap = sharedheap_get(sizeof(struct mychan));
	chanacs_heap = sharedheap_get(sizeof(struct chanacs));
	mycertfp_heap = sharedheap_get(sizeof(struct mycertfp));

	if (myuser_heap == NULL || mynick_heap == NULL || mychan_heap == NULL
			|| chanacs_heap == NULL || mycertfp_heap == NULL)
	{
		slog(LG_ERROR, "init_accounts(): block allocator failure.");
		exit(EXIT_FAILURE);
	}

	nicklist = mowgli_patricia_create(irccasecanon);
	oldnameslist = mowgli_patricia_create(irccasecanon);
	mclist = mowgli_patricia_create(irccasecanon);
	certfplist = mowgli_patricia_create(strcasecanon);
}

/*
 * myuser_add(const char *name, const char *pass, const char *email,
 * unsigned int flags)
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
 *      - on success, a new struct myuser object (account)
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

struct myuser *
myuser_add(const char *name, const char *pass, const char *email, unsigned int flags)
{
	return myuser_add_id(NULL, name, pass, email, flags);
}

/*
 * myuser_add_id(const char *id, const char *name, const char *pass,
 * const char *email, unsigned int flags)
 *
 * Like myuser_add, but lets you specify the new entity's UID.
 */
struct myuser *
myuser_add_id(const char *id, const char *name, const char *pass, const char *email, unsigned int flags)
{
	struct myuser *mu;
	struct soper *soper;

	return_val_if_fail((mu = myuser_find(name)) == NULL, mu);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "myuser_add(): %s -> %s", name, email);

	mu = mowgli_heap_alloc(myuser_heap);
	atheme_object_init(atheme_object(mu), name, (atheme_object_destructor_fn) myuser_delete);

	entity(mu)->type = ENT_USER;
	entity(mu)->name = strshare_get(name);
	mu->email = strshare_get(email);
	mu->email_canonical = canonicalize_email(email);
	if (id)
	{
		if (myentity_find_uid(id) == NULL)
			mowgli_strlcpy(entity(mu)->id, id, sizeof(entity(mu)->id));
		else
			entity(mu)->id[0] = '\0';
	}
	else
		entity(mu)->id[0] = '\0';

	mu->registered = CURRTIME;
	mu->flags = flags;
	if (mu->flags & MU_ENFORCE)
	{
		mu->flags &= ~MU_ENFORCE;
		metadata_add(mu, "private:doenforce", "1");
	}
	mu->language = NULL; /* default */

	/* If it's already crypted, don't touch the password. Otherwise,
	 * use set_password() to initialize it. Why? Because set_password
	 * will move the user to encrypted passwords if possible. That way,
	 * new registers are immediately protected and the database is
	 * immediately converted the first time we start up with crypto.
	 */
	if (flags & MU_CRYPTPASS)
		mowgli_strlcpy(mu->pass, pass, sizeof mu->pass);
	else
		set_password(mu, pass);

	myentity_put(entity(mu));

	if ((soper = soper_find_named(entity(mu)->name)) != NULL)
	{
		slog(LG_DEBUG, "myuser_add(): user `%s' has been declared as soper, activating privileges.", entity(mu)->name);
		soper->myuser = mu;
		mu->soper = soper;
	}

	myuser_name_restore(entity(mu)->name, mu);

	cnt.myuser++;

	return mu;
}

/*
 * myuser_delete(struct myuser *mu)
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
void
myuser_delete(struct myuser *mu)
{
	struct myuser *successor;
	struct mychan *mc;
	struct mynick *mn;
	struct user *u;
	mowgli_node_t *n, *tn;
	struct mymemo *memo;
	struct chanacs *ca;
	char nicks[200];

	return_if_fail(mu != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "myuser_delete(): %s", entity(mu)->name);

	myuser_name_remember(entity(mu)->name, mu);

	hook_call_myuser_delete(mu);

	/* log them out */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->logins.head)
	{
		u = (struct user *)n->data;
		if (!authservice_loaded || !ircd_logout_or_kill(u, entity(mu)->name))
		{
			u->myuser = NULL;
			mowgli_node_delete(n, &mu->logins);
			mowgli_node_free(n);
		}
	}

	/* kill all their channels and chanacs */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, entity(mu)->chanacs.head)
	{
		ca = n->data;
		mc = ca->mychan;

		/* attempt succession */
		if (ca->level & CA_FOUNDER && mychan_num_founders(mc) == 1 && (successor = mychan_pick_successor(mc)) != NULL)
		{
			slog(LG_INFO, "SUCCESSION: \2%s\2 to \2%s\2 from \2%s\2", mc->name, entity(successor)->name, entity(mu)->name);
			slog(LG_VERBOSE, "myuser_delete(): giving channel %s to %s (unused %lds, founder %s, chanacs %zu)",
					mc->name, entity(successor)->name,
					(long)(CURRTIME - mc->used),
					entity(mu)->name,
					MOWGLI_LIST_LENGTH(&mc->chanacs));
			if (chansvs.me != NULL)
				verbose(mc, "Foundership changed to \2%s\2 because \2%s\2 was dropped.", entity(successor)->name, entity(mu)->name);

			/* CA_FOUNDER | CA_FLAGS is the minimum required for full control; let chanserv take care of assigning the rest via founder_flags */
			chanacs_change_simple(mc, entity(successor), NULL, CA_FOUNDER | CA_FLAGS, 0, NULL);
			hook_call_channel_succession((&(struct hook_channel_succession_req){ .mc = mc, .mu = successor }));

			if (chansvs.me != NULL)
				myuser_notice(chansvs.nick, successor, "You are now founder on \2%s\2 (as \2%s\2).", mc->name, entity(successor)->name);
			atheme_object_unref(ca);
		}
		/* no successor found */
		else if (ca->level & CA_FOUNDER && mychan_num_founders(mc) == 1)
		{
			slog(LG_REGISTER, "DELETE: \2%s\2 from \2%s\2", mc->name, entity(mu)->name);
			slog(LG_VERBOSE, "myuser_delete(): deleting channel %s (unused %lds, founder %s, chanacs %zu)",
					mc->name, (long)(CURRTIME - mc->used),
					entity(mu)->name,
					MOWGLI_LIST_LENGTH(&mc->chanacs));

			hook_call_channel_drop(mc);
			if (mc->chan != NULL && !(mc->chan->flags & CHAN_LOG))
				part(mc->name, chansvs.nick);
			atheme_object_unref(mc);
		}
		else /* not founder */
			atheme_object_unref(ca);
	}

	/* remove them from the soper list */
	if (soper_find(mu))
		soper_delete(mu->soper);

	metadata_delete_all(mu);

	/* kill any authcookies */
	authcookie_destroy_all(mu);

	/* delete memos */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->memos.head)
	{
		memo = (struct mymemo *)n->data;

		mowgli_node_delete(n, &mu->memos);
		mowgli_node_free(n);
		sfree(memo);
	}

	/* delete access entries */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->access_list.head)
		myuser_access_delete(mu, (char *)n->data);

	/* delete certfp entries */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->cert_fingerprints.head)
		mycertfp_delete((struct mycertfp *) n->data);

	/* delete their nicks and report them */
	nicks[0] = '\0';
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->nicks.head)
	{
		mn = n->data;
		if (irccasecmp(mn->nick, entity(mu)->name))
		{
			slog(LG_VERBOSE, "myuser_delete(): deleting nick %s (unused %lds, owner %s)",
					mn->nick,
					(long)(CURRTIME - mn->lastseen),
					entity(mu)->name);
			if (strlen(nicks) + strlen(mn->nick) + 3 >= sizeof nicks)
			{
				slog(LG_REGISTER, "DELETE: \2%s\2 from \2%s\2", nicks, entity(mu)->name);
				nicks[0] = '\0';
			}
			if (nicks[0] != '\0')
				mowgli_strlcat(nicks, ", ", sizeof nicks);
			mowgli_strlcat(nicks, "\2", sizeof nicks);
			mowgli_strlcat(nicks, mn->nick, sizeof nicks);
			mowgli_strlcat(nicks, "\2", sizeof nicks);
		}
		atheme_object_unref(mn);
	}
	if (nicks[0] != '\0')
		slog(LG_REGISTER, "DELETE: \2%s\2 from \2%s\2", nicks, entity(mu)->name);

	/* entity(mu)->name is the index for this dtree */
	myentity_del(entity(mu));

	strshare_unref(mu->email);
	strshare_unref(mu->email_canonical);
	strshare_unref(entity(mu)->name);

	mowgli_heap_free(myuser_heap, mu);

	cnt.myuser--;
}

/*
 * myuser_rename(struct myuser *mu, const char *name)
 *
 * Renames an account in the accounts DTree.
 *
 * Inputs:
 *      - account to rename
 *      - new name; if nickname ownership is enabled, this must be
 *        irccmp-equal to a nick registered to the account
 *
 * Outputs:
 *      - nothing
 *
 * Side Effects:
 *      - an account is renamed.
 *      - online users are logged out and in again
 */
void
myuser_rename(struct myuser *mu, const char *name)
{
	mowgli_node_t *n, *tn;
	struct user *u;
	struct hook_user_rename data;
	stringref newname;
	char nb[NICKLEN + 1];

	return_if_fail(mu != NULL);
	return_if_fail(name != NULL);
	return_if_fail(strlen(name) < sizeof nb);

	mowgli_strlcpy(nb, entity(mu)->name, sizeof nb);
	newname = strshare_get(name);

	if (authservice_loaded)
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->logins.head)
		{
			u = n->data;
			ircd_on_logout(u, entity(mu)->name);
		}
	}
	myentity_del(entity(mu));

	strshare_unref(entity(mu)->name);
	entity(mu)->name = newname;

	myentity_put(entity(mu));
	if (authservice_loaded && !(mu->flags & MU_WAITAUTH))
	{
		MOWGLI_ITER_FOREACH(n, mu->logins.head)
		{
			u = n->data;
			ircd_on_login(u, mu, NULL);
		}
	}

	data.mu = mu;
	data.oldname = nb;
	hook_call_user_rename(&data);
}

/*
 * myuser_set_email(struct myuser *mu, const char *name)
 *
 * Changes the email address of an account.
 *
 * Inputs:
 *      - account to change
 *      - new email address; this must be valid
 *
 * Outputs:
 *      - nothing
 *
 * Side Effects:
 *      - email address is changed
 */
void
myuser_set_email(struct myuser *mu, const char *newemail)
{
	return_if_fail(mu != NULL);
	return_if_fail(newemail != NULL);

	strshare_unref(mu->email);
	strshare_unref(mu->email_canonical);

	mu->email = strshare_get(newemail);
	mu->email_canonical = canonicalize_email(newemail);
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
struct myuser *
myuser_find_ext(const char *name)
{
	struct user *u;
	struct mynick *mn;

	return_val_if_fail(name != NULL, NULL);

	if (*name == '=')
	{
		u = user_find_named(name + 1);
		return u != NULL ? u->myuser : NULL;
	}
	else if (*name == '?')
	{
		return myuser_find_uid(name + 1);
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
 * myuser_notice(const char *from, struct myuser *target, const char *fmt, ...)
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
void ATHEME_FATTR_PRINTF(3, 4)
myuser_notice(const char *from, struct myuser *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	mowgli_node_t *n;
	struct user *u;

	return_if_fail(from != NULL);
	return_if_fail(target != NULL);
	return_if_fail(fmt != NULL);

	/* have to reformat it here, can't give a va_list to notice() :(
	 * -- jilles
	 */
	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	MOWGLI_ITER_FOREACH(n, target->logins.head)
	{
		u = (struct user *)n->data;
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
 *     - true if user matches an accesslist entry
 *     - false otherwise
 *
 * Side Effects:
 *     - none
 */
bool
myuser_access_verify(struct user *u, struct myuser *mu)
{
	mowgli_node_t *n;
	char buf[USERLEN + 1 + HOSTLEN + 1];
	char buf2[USERLEN + 1 + HOSTLEN + 1];
	char buf3[USERLEN + 1 + HOSTLEN + 1];
	char buf4[USERLEN + 1 + HOSTLEN + 1];

	return_val_if_fail(u != NULL, false);
	return_val_if_fail(mu != NULL, false);

	if (!use_myuser_access)
		return false;

	if (metadata_find(mu, "private:freeze:freezer"))
		return false;

	snprintf(buf, sizeof buf, "%s@%s", u->user, u->vhost);
	snprintf(buf2, sizeof buf2, "%s@%s", u->user, u->host);
	snprintf(buf3, sizeof buf3, "%s@%s", u->user, u->ip);
	snprintf(buf4, sizeof buf4, "%s@%s", u->user, u->chost);

	MOWGLI_ITER_FOREACH(n, mu->access_list.head)
	{
		char *entry = (char *) n->data;

		if (!match(entry, buf) || !match(entry, buf2) || !match(entry, buf3) || !match(entry, buf4) || !match_cidr(entry, buf3))
			return true;
	}

	return false;
}

/*
 * myuser_access_add()
 *
 * Inputs:
 *     - account to attach access mask to, access mask itself
 *
 * Outputs:
 *     - false: me.mdlimit is reached (too many access entries)
 *     - true : success
 *
 * Side Effects:
 *     - an access mask is added to an account.
 */
bool
myuser_access_add(struct myuser *mu, const char *mask)
{
	mowgli_node_t *n;
	char *msk;

	return_val_if_fail(mu != NULL, false);
	return_val_if_fail(mask != NULL, false);

	if (MOWGLI_LIST_LENGTH(&mu->access_list) > me.mdlimit)
	{
		slog(LG_DEBUG, "myuser_access_add(): access entry limit reached for %s", entity(mu)->name);
		return false;
	}

	msk = sstrdup(mask);
	n = mowgli_node_create();
	mowgli_node_add(msk, n, &mu->access_list);

	cnt.myuser_access++;

	return true;
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
myuser_access_find(struct myuser *mu, const char *mask)
{
	mowgli_node_t *n;

	return_val_if_fail(mu != NULL, NULL);
	return_val_if_fail(mask != NULL, NULL);

	MOWGLI_ITER_FOREACH(n, mu->access_list.head)
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
myuser_access_delete(struct myuser *mu, const char *mask)
{
	mowgli_node_t *n, *tn;

	return_if_fail(mu != NULL);
	return_if_fail(mask != NULL);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->access_list.head)
	{
		char *entry = (char *) n->data;

		if (!strcasecmp(entry, mask))
		{
			mowgli_node_delete(n, &mu->access_list);
			mowgli_node_free(n);
			sfree(entry);

			cnt.myuser_access--;

			return;
		}
	}
}

/***************
 * M Y N I C K *
 ***************/

/*
 * mynick_add(struct myuser *mu, const char *name)
 *
 * Creates a nick registration for the given account and adds it to the
 * nicks DTree.
 *
 * Inputs:
 *      - an account pointer
 *      - a nickname
 *
 * Outputs:
 *      - on success, a new struct mynick object
 *      - on failure, NULL.
 *
 * Side Effects:
 *      - the created nick is added to the nick DTree and to the
 *        account's list.
 */
struct mynick *
mynick_add(struct myuser *mu, const char *name)
{
	struct mynick *mn;

	return_val_if_fail((mn = mynick_find(name)) == NULL, mn);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "mynick_add(): %s -> %s", name, entity(mu)->name);

	mn = mowgli_heap_alloc(mynick_heap);
	atheme_object_init(atheme_object(mn), name, (atheme_object_destructor_fn) mynick_delete);

	mowgli_strlcpy(mn->nick, name, sizeof mn->nick);
	mn->owner = mu;
	mn->registered = CURRTIME;

	mowgli_patricia_add(nicklist, mn->nick, mn);
	mowgli_node_add(mn, &mn->node, &mu->nicks);

	myuser_name_restore(mn->nick, mu);

	cnt.mynick++;

	return mn;
}

/*
 * mynick_delete(struct mynick *mn)
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
void
mynick_delete(struct mynick *mn)
{
	return_if_fail(mn != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "mynick_delete(): %s", mn->nick);

	myuser_name_remember(mn->nick, mn->owner);

	mowgli_patricia_delete(nicklist, mn->nick);
	mowgli_node_delete(&mn->node, &mn->owner->nicks);

	mowgli_heap_free(mynick_heap, mn);

	cnt.mynick--;
}

/*************************
 * M Y U S E R _ N A M E *
 *************************/

static void myuser_name_delete(struct myuser_name *mun);

/*
 * myuser_name_add(const char *name)
 *
 * Creates a record for the given name and adds it to the oldnames DTree.
 *
 * Inputs:
 *      - a nick or account name
 *
 * Outputs:
 *      - on success, a new struct myuser_name object
 *      - on failure, NULL.
 *
 * Side Effects:
 *      - the created record is added to the oldnames DTree.
 */
struct myuser_name *
myuser_name_add(const char *name)
{
	struct myuser_name *mun;

	return_val_if_fail((mun = myuser_name_find(name)) == NULL, mun);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "myuser_name_add(): %s", name);

	mun = mowgli_heap_alloc(myuser_name_heap);
	atheme_object_init(atheme_object(mun), name, (atheme_object_destructor_fn) myuser_name_delete);

	mowgli_strlcpy(mun->name, name, sizeof mun->name);

	mowgli_patricia_add(oldnameslist, mun->name, mun);

	cnt.myuser_name++;

	return mun;
}

/*
 * myuser_name_delete(struct myuser_name *mun)
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
static void
myuser_name_delete(struct myuser_name *mun)
{
	return_if_fail(mun != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "myuser_name_delete(): %s", mun->name);

	mowgli_patricia_delete(oldnameslist, mun->name);

	metadata_delete_all(mun);

	mowgli_heap_free(myuser_name_heap, mun);

	cnt.myuser_name--;
}

/*
 * myuser_name_remember(const char *name, struct myuser *mu)
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
void
myuser_name_remember(const char *name, struct myuser *mu)
{
	struct myuser_name *mun;
	struct metadata *md;

	if (myuser_name_find(name))
		return;

	md = metadata_find(mu, "private:mark:setter");
	if (md == NULL)
		return;

	mun = myuser_name_add(name);

	metadata_add(mun, md->name, md->value);
	md = metadata_find(mu, "private:mark:reason");
	if (md != NULL)
		metadata_add(mun, md->name, md->value);
	md = metadata_find(mu, "private:mark:timestamp");
	if (md != NULL)
		metadata_add(mun, md->name, md->value);

	return;
}

/*
 * myuser_name_restore(const char *name, struct myuser *mu)
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
void
myuser_name_restore(const char *name, struct myuser *mu)
{
	struct myuser_name *mun;
	struct metadata *md, *md2;
	mowgli_patricia_iteration_state_t state;
	char *copy;

	mun = myuser_name_find(name);
	if (mun == NULL)
		return;

	md = metadata_find(mu, "private:mark:reason");
	md2 = metadata_find(mun, "private:mark:reason");
	if (md != NULL && md2 != NULL && strcmp(md->value, md2->value))
	{
		wallops("Not restoring mark \2\"%s\"\2 for account \2%s\2 (name \2%s\2) which is already marked", md2->value, entity(mu)->name, name);
		slog(LG_INFO, "MARK:FORGET: \2\"%s\"\2 for \2%s (%s)\2 (already marked)", md2->value, name, entity(mu)->name);
		slog(LG_VERBOSE, "myuser_name_restore(): not restoring mark \"%s\" for account %s (name %s) which is already marked",
				md2->value, entity(mu)->name, name);
	}
	else if (md == NULL && md2 != NULL)
	{
		slog(LG_INFO, "MARK:RESTORE: \2\"%s\"\2 for \2%s (%s)\2", md2->value, name, entity(mu)->name);
		slog(LG_VERBOSE, "myuser_name_restore(): restoring mark \"%s\" for account %s (name %s)",
				md2->value, entity(mu)->name, name);
	}

	if (atheme_object(mun)->metadata)
	{
		MOWGLI_PATRICIA_FOREACH(md, &state, atheme_object(mun)->metadata)
		{
			/* prefer current metadata to saved */
			if (!metadata_find(mu, md->name))
			{
				if (strcmp(md->name, "private:mark:reason") ||
						!strncmp(md->value, "(restored) ", 11))
					metadata_add(mu, md->name, md->value);
				else
				{
					copy = smalloc(strlen(md->value) + 12);
					memcpy(copy, "(restored) ", 11);
					strcpy(copy + 11, md->value);
					metadata_add(mu, md->name, copy);
					sfree(copy);
				}
			}
		}
	}

	atheme_object_unref(mun);

	return;
}

/*******************
 * M Y C E R T F P *
 *******************/

struct mycertfp *
mycertfp_add(struct myuser *mu, const char *certfp, const bool force)
{
	return_val_if_fail(mu != NULL, NULL);
	return_val_if_fail(certfp != NULL, NULL);

	if (me.maxcertfp && MOWGLI_LIST_LENGTH(&mu->cert_fingerprints) >= me.maxcertfp && ! force)
		return NULL;

	struct mycertfp *const mcfp = mowgli_heap_alloc(mycertfp_heap);

	mcfp->mu = mu;
	mcfp->certfp = sstrdup(certfp);

	(void) mowgli_node_add(mcfp, &mcfp->node, &mu->cert_fingerprints);
	(void) mowgli_patricia_add(certfplist, mcfp->certfp, mcfp);

	return mcfp;
}

void
mycertfp_delete(struct mycertfp *mcfp)
{
	return_if_fail(mcfp != NULL);
	return_if_fail(mcfp->mu != NULL);
	return_if_fail(mcfp->certfp != NULL);

	mowgli_node_delete(&mcfp->node, &mcfp->mu->cert_fingerprints);
	mowgli_patricia_delete(certfplist, mcfp->certfp);

	sfree(mcfp->certfp);
	mowgli_heap_free(mycertfp_heap, mcfp);
}

struct mycertfp *
mycertfp_find(const char *certfp)
{
	return_val_if_fail(certfp != NULL, NULL);

	return mowgli_patricia_retrieve(certfplist, certfp);
}

/***************
 * M Y C H A N *
 ***************/

/* private destructor for struct mychan. */
static void
mychan_delete(struct mychan *mc)
{
	mowgli_node_t *n, *tn;

	return_if_fail(mc != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "mychan_delete(): %s", mc->name);

	if (mc->chan != NULL)
		mc->chan->mychan = NULL;

	/* remove the chanacs shiz */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chanacs.head)
		atheme_object_unref(n->data);

	metadata_delete_all(mc);

	mowgli_patricia_delete(mclist, mc->name);

	strshare_unref(mc->name);

	mowgli_heap_free(mychan_heap, mc);

	cnt.mychan--;
}

struct mychan *
mychan_add(char *name)
{
	struct mychan *mc;

	return_val_if_fail(name != NULL, NULL);
	return_val_if_fail((mc = mychan_find(name)) == NULL, mc);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "mychan_add(): %s", name);

	mc = mowgli_heap_alloc(mychan_heap);

	atheme_object_init(atheme_object(mc), name, (atheme_object_destructor_fn) mychan_delete);
	mc->name = strshare_get(name);
	mc->registered = CURRTIME;
	mc->chan = channel_find(name);

	if (mc->chan != NULL)
		mc->chan->mychan = mc;

	mowgli_patricia_add(mclist, mc->name, mc);

	cnt.mychan++;

	return mc;
}


/* Check if there is anyone on the channel fulfilling the conditions.
 * Fairly expensive, but this is sometimes necessary to avoid
 * inappropriate drops. -- jilles */
bool
mychan_isused(struct mychan *mc)
{
	mowgli_node_t *n;
	struct channel *c;
	struct chanuser *cu;

	return_val_if_fail(mc != NULL, false);

	c = mc->chan;
	if (c == NULL)
		return false;
	MOWGLI_ITER_FOREACH(n, c->members.head)
	{
		cu = n->data;
		if (chanacs_user_flags(mc, cu->user) & CA_USEDUPDATE)
			return true;
	}
	return false;
}

unsigned int
mychan_num_founders(struct mychan *mc)
{
	mowgli_node_t *n;
	struct chanacs *ca;
	unsigned int count = 0;

	return_val_if_fail(mc != NULL, 0);

	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;
		if (ca->entity != NULL && ca->level & CA_FOUNDER)
			count++;
	}
	return count;
}

const char *
mychan_founder_names(struct mychan *mc)
{
	mowgli_node_t *n;
	struct chanacs *ca;
	static char names[512];

	return_val_if_fail(mc != NULL, NULL);

	names[0] = '\0';
	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;
		if (ca->entity != NULL && ca->level & CA_FOUNDER)
		{
			if (names[0] != '\0')
				mowgli_strlcat(names, ", ", sizeof names);
			mowgli_strlcat(names, entity(ca->entity)->name, sizeof names);
		}
	}
	return names;
}

static unsigned int
add_auto_flags(unsigned int flags)
{
	if (flags & CA_OP)
		flags |= CA_AUTOOP;
	if (flags & CA_HALFOP)
		flags |= CA_AUTOHALFOP;
	if (flags & CA_VOICE)
		flags |= CA_AUTOVOICE;
	return flags;
}

/* When to consider a user recently seen */
#define RECENTLY_SEEN SECONDS_PER_WEEK

/* Find a user fulfilling the conditions who can take another channel */
struct myuser *
mychan_pick_candidate(struct mychan *mc, unsigned int minlevel)
{
	mowgli_node_t *n;
	struct chanacs *ca;
	struct myentity *mt, *hi_mt;
	unsigned int level, hi_level;
	bool recent_ok = false;
	bool hi_recent_ok = false;

	return_val_if_fail(mc != NULL, NULL);

	hi_mt = NULL;
	hi_level = 0;
	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;
		if (ca->level & CA_AKICK)
			continue;
		mt = ca->entity;
		if (mt == NULL || ca->level & CA_FOUNDER)
			continue;
		if ((ca->level & minlevel) != minlevel)
			continue;

		/* now have a user with requested flags */
		if (isuser(mt))
			recent_ok = MOWGLI_LIST_LENGTH(&user(mt)->logins) > 0 ||
				CURRTIME - user(mt)->lastlogin < RECENTLY_SEEN;

		level = add_auto_flags(ca->level);
		/* compare with previous candidate, this user must be
		 * "better" to beat the previous. this means that ties
		 * are broken in favour of the user added first.
		 */
		if (hi_mt != NULL)
		{
			/* previous candidate has flags this user doesn't? */
			if (hi_level & ~level)
				continue;
			/* if equal flags, recent_ok must be better */
			if (hi_level == level && (!recent_ok || hi_recent_ok))
				continue;
		}
		if (myentity_can_register_channel(mt))
		{
			hi_mt = mt;
			hi_level = level;
			hi_recent_ok = recent_ok;
		}
	}
	return user(hi_mt);
}

/* Pick a suitable successor
 * Note: please do not make this dependent on currently being in
 * the channel or on IRC; this would give an unfair advantage to
 * 24*7 clients and bots.
 * -- jilles */
struct myuser *
mychan_pick_successor(struct mychan *mc)
{
	struct myuser *mu;
	struct hook_channel_succession_req req;

	return_val_if_fail(mc != NULL, NULL);

	/* allow a hook to override/augment the succession. */
	req.mc = mc;
	req.mu = NULL;
	hook_call_channel_pick_successor(&req);
	if (req.mu != NULL)
		return req.mu;

	/* value +R higher than other flags
	 * (old successor has this, but not sop, and help file mentions this)
	 */
	mu = mychan_pick_candidate(mc, CA_RECOVER);
	if (mu != NULL)
		return mu;
	/* +f is also a powerful flag */
	mu = mychan_pick_candidate(mc, CA_FLAGS);
	if (mu != NULL)
		return mu;
	/* I guess +q means something */
	if (ca_all & CA_USEOWNER)
	{
		mu = mychan_pick_candidate(mc, CA_USEOWNER | CA_AUTOOP);
		if (mu != NULL)
			return mu;
	}
	/* an op perhaps? */
	mu = mychan_pick_candidate(mc, CA_OP);
	if (mu != NULL)
		return mu;
	/* just a user with access */
	return mychan_pick_candidate(mc, 0);
}

const char *
mychan_get_mlock(struct mychan *mc)
{
	static char buf[BUFSIZE];
	char params[BUFSIZE];
	struct metadata *md;
	char *p, *q, *qq;
	int dir;

	return_val_if_fail(mc != NULL, NULL);

	*buf = 0;
	*params = 0;

	md = metadata_find(mc, "private:mlockext");
	if (md != NULL && strlen(md->value) > 450)
		md = NULL;

	dir = MTYPE_NUL;

	if (mc->mlock_on)
	{
		if (dir != MTYPE_ADD)
		{
			dir = MTYPE_ADD;
			mowgli_strlcat(buf, "+", sizeof buf);
		}
		mowgli_strlcat(buf, flags_to_string(mc->mlock_on), sizeof buf);
	}

	if (mc->mlock_limit)
	{
		if (dir != MTYPE_ADD)
		{
			dir = MTYPE_ADD;
			mowgli_strlcat(buf, "+", sizeof buf);
		}
		mowgli_strlcat(buf, "l", sizeof buf);
		mowgli_strlcat(params, " ", sizeof params);
		mowgli_strlcat(params, number_to_string(mc->mlock_limit), sizeof params);
	}

	if (mc->mlock_key)
	{
		if (dir != MTYPE_ADD)
		{
			dir = MTYPE_ADD;
			mowgli_strlcat(buf, "+", sizeof buf);
		}
		mowgli_strlcat(buf, "k", sizeof buf);
		mowgli_strlcat(params, " *", sizeof params);
	}

	if (md)
	{
		p = md->value;
		q = buf + strlen(buf);
		while (*p != '\0')
		{
			if (p[1] != ' ' && p[1] != '\0')
			{
				if (dir != MTYPE_ADD)
				{
					dir = MTYPE_ADD;
					*q++ = '+';
				}
				*q++ = *p++;
				mowgli_strlcat(params, " ", sizeof params);
				qq = params + strlen(params);
				while (*p != '\0' && *p != ' ')
					*qq++ = *p++;
				*qq = '\0';
			}
			else
			{
				p++;
				while (*p != '\0' && *p != ' ')
					p++;
			}
			while (*p == ' ')
				p++;
		}
		*q = '\0';
	}

	if (mc->mlock_off)
	{
		if (dir != MTYPE_DEL)
		{
			dir = MTYPE_DEL;
			mowgli_strlcat(buf, "-", sizeof buf);
		}
		mowgli_strlcat(buf, flags_to_string(mc->mlock_off), sizeof buf);
		if (mc->mlock_off & CMODE_LIMIT)
			mowgli_strlcat(buf, "l", sizeof buf);
		if (mc->mlock_off & CMODE_KEY)
			mowgli_strlcat(buf, "k", sizeof buf);
	}

	if (md)
	{
		p = md->value;
		q = buf + strlen(buf);
		while (*p != '\0')
		{
			if (p[1] == ' ' || p[1] == '\0')
			{
				if (dir != MTYPE_DEL)
				{
					dir = MTYPE_DEL;
					*q++ = '-';
				}
				*q++ = *p;
			}
			p++;
			while (*p != '\0' && *p != ' ')
				p++;
			while (*p == ' ')
				p++;
		}
		*q = '\0';
	}

	mowgli_strlcat(buf, params, BUFSIZE);

	if (*buf == '\0')
		mowgli_strlcpy(buf, "+", BUFSIZE);

	return buf;
}

const char *
mychan_get_sts_mlock(struct mychan *mc)
{
	static char mlock[BUFSIZE];
	struct metadata *md;

	return_val_if_fail(mc != NULL, NULL);

	mlock[0] = '\0';

	if (mc->mlock_on)
		mowgli_strlcat(mlock, flags_to_string(mc->mlock_on), sizeof(mlock));
	if (mc->mlock_off)
		mowgli_strlcat(mlock, flags_to_string(mc->mlock_off), sizeof(mlock));
	if (mc->mlock_limit || mc->mlock_off & CMODE_LIMIT)
		mowgli_strlcat(mlock, "l", sizeof(mlock));
	if (mc->mlock_key || mc->mlock_off & CMODE_KEY)
		mowgli_strlcat(mlock, "k", sizeof(mlock));
	if ((md = metadata_find(mc, "private:mlockext")))
	{
		char *p = md->value;
		char *q = mlock + strlen(mlock);
		while (*p != '\0')
		{
			*q++ = *p++;
			while (*p != ' ' && *p != '\0')
				p++;
			while (*p == ' ')
				p++;
		}
		*q = '\0';
	}

	return mlock;
}

/*****************
 * C H A N A C S *
 *****************/

/* private destructor for struct chanacs */
static void
chanacs_delete(struct chanacs *ca)
{
	return_if_fail(ca != NULL);
	return_if_fail(ca->mychan != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "chanacs_delete(): %s -> %s [%s]", ca->mychan->name,
			ca->entity != NULL ? entity(ca->entity)->name : ca->host,
			ca->entity != NULL ? "entity" : "hostmask");
	mowgli_node_delete(&ca->cnode, &ca->mychan->chanacs);

	if (ca->entity != NULL)
	{
		mowgli_node_delete(&ca->unode, &ca->entity->chanacs);

		if (isdynamic(ca->entity))
			atheme_object_unref(ca->entity);
	}

	metadata_delete_all(ca);

	sfree(ca->host);

	mowgli_heap_free(chanacs_heap, ca);

	cnt.chanacs--;
}

/*
 * chanacs_add(struct mychan *mychan, struct myuser *myuser, unsigned int level, time_t ts, struct myentity *setter)
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
 *       - a struct chanacs object which describes the mapping
 *
 * Side Effects:
 *       - the channel access list is updated for mychan.
 */
struct chanacs *
chanacs_add(struct mychan *mychan, struct myentity *mt, unsigned int level, time_t ts, struct myentity *setter)
{
	struct chanacs *ca;

	return_val_if_fail(mychan != NULL && mt != NULL, NULL);

	if (*mychan->name != '#')
	{
		slog(LG_DEBUG, "chanacs_add(): got non #channel: %s", mychan->name);
		return NULL;
	}

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "chanacs_add(): %s -> %s", mychan->name, mt->name);

	ca = mowgli_heap_alloc(chanacs_heap);

	atheme_object_init(atheme_object(ca), mt->name, (atheme_object_destructor_fn) chanacs_delete);
	ca->mychan = mychan;
	ca->entity = isdynamic(mt) ? atheme_object_ref(mt) : mt;
	ca->host = NULL;
	ca->level = level & ca_all;
	ca->tmodified = ts;

	if (setter != NULL)
		mowgli_strlcpy(ca->setter_uid, setter->id, sizeof ca->setter_uid);
	else
		ca->setter_uid[0] = '\0';

	mowgli_node_add(ca, &ca->cnode, &mychan->chanacs);
	mowgli_node_add(ca, &ca->unode, &mt->chanacs);

	cnt.chanacs++;

	return ca;
}

/*
 * chanacs_add_host(struct mychan *mychan, char *host, unsigned int level, time_t ts)
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
 *       - a struct chanacs object which describes the mapping
 *
 * Side Effects:
 *       - the channel access list is updated for mychan.
 */
struct chanacs *
chanacs_add_host(struct mychan *mychan, const char *host, unsigned int level, time_t ts, struct myentity *setter)
{
	struct chanacs *ca;

	return_val_if_fail(mychan != NULL && host != NULL, NULL);

	if (*mychan->name != '#')
	{
		slog(LG_DEBUG, "chanacs_add_host(): got non #channel: %s", mychan->name);
		return NULL;
	}

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "chanacs_add_host(): %s -> %s", mychan->name, host);

	ca = mowgli_heap_alloc(chanacs_heap);

	atheme_object_init(atheme_object(ca), host, (atheme_object_destructor_fn) chanacs_delete);
	ca->mychan = mychan;
	ca->entity = NULL;
	ca->host = sstrdup(host);
	ca->level = level & ca_all;
	ca->tmodified = ts;

	if (setter != NULL)
		mowgli_strlcpy(ca->setter_uid, setter->id, sizeof ca->setter_uid);
	else
		ca->setter_uid[0] = '\0';

	mowgli_node_add(ca, &ca->cnode, &mychan->chanacs);

	cnt.chanacs++;

	return ca;
}

struct chanacs *
chanacs_find(struct mychan *mychan, struct myentity *mt, unsigned int level)
{
	mowgli_node_t *n;
	struct chanacs *ca;

	return_val_if_fail(mychan != NULL && mt != NULL, NULL);

	if ((ca = chanacs_find_literal(mychan, mt, level)) != NULL)
		return ca;

	MOWGLI_ITER_FOREACH(n, mychan->chanacs.head)
	{
		const struct entity_vtable *vt;

		ca = (struct chanacs *)n->data;

		if (ca->entity == NULL)
			continue;

		vt = myentity_get_vtable(ca->entity);
		if (level != 0x0)
		{
			if (vt->match_entity(ca->entity, mt) && (ca->level & level) == level)
				return ca;
		}
		else if (vt->match_entity(ca->entity, mt))
			return ca;
	}

	return NULL;
}

unsigned int
chanacs_entity_flags(struct mychan *mychan, struct myentity *mt)
{
	mowgli_node_t *n;
	struct chanacs *ca;
	unsigned int result = 0;

	return_val_if_fail(mychan != NULL && mt != NULL, 0);

	MOWGLI_ITER_FOREACH(n, mychan->chanacs.head)
	{
		const struct entity_vtable *vt;

		ca = (struct chanacs *)n->data;

		if (ca->entity == NULL)
			continue;
		if (ca->entity == mt)
			result |= ca->level;
		else
		{
			vt = myentity_get_vtable(ca->entity);
			if (vt->match_entity(ca->entity, mt))
				result |= ca->level;
		}
	}

	slog(LG_DEBUG, "chanacs_entity_flags(%s, %s): return %s", mychan->name, mt->name, bitmask_to_flags(result));

	return result;
}

struct chanacs *
chanacs_find_literal(struct mychan *mychan, struct myentity *mt, unsigned int level)
{
	mowgli_node_t *n;
	struct chanacs *ca;

	return_val_if_fail(mychan != NULL && mt != NULL, NULL);

	MOWGLI_ITER_FOREACH(n, mychan->chanacs.head)
	{
		ca = (struct chanacs *)n->data;

		if (level != 0x0)
		{
			if (ca->entity == mt && ((ca->level & level) == level))
				return ca;
		}
		else if (ca->entity == mt)
			return ca;
	}

	return NULL;
}

struct chanacs *
chanacs_find_host(struct mychan *mychan, const char *host, unsigned int level)
{
	mowgli_node_t *n;
	struct chanacs *ca;

	return_val_if_fail(mychan != NULL && host != NULL, NULL);

	MOWGLI_ITER_FOREACH(n, mychan->chanacs.head)
	{
		ca = (struct chanacs *)n->data;

		if (level != 0x0)
		{
			if ((ca->entity == NULL) && (!match(ca->host, host)) && ((ca->level & level) == level))
				return ca;
		}
		else if ((ca->entity == NULL) && (!match(ca->host, host)))
			return ca;
	}

	return NULL;
}

unsigned int
chanacs_host_flags(struct mychan *mychan, const char *host)
{
	mowgli_node_t *n;
	struct chanacs *ca;
	unsigned int result = 0;

	return_val_if_fail(mychan != NULL && host != NULL, 0);

	MOWGLI_ITER_FOREACH(n, mychan->chanacs.head)
	{
		ca = (struct chanacs *)n->data;

		if (ca->entity == NULL && !match(ca->host, host))
			result |= ca->level;
	}

	return result;
}

struct chanacs *
chanacs_find_host_literal(struct mychan *mychan, const char *host, unsigned int level)
{
	mowgli_node_t *n;
	struct chanacs *ca;

	if ((!mychan) || (!host))
		return NULL;

	MOWGLI_ITER_FOREACH(n, mychan->chanacs.head)
	{
		ca = (struct chanacs *)n->data;

		if (level != 0x0)
		{
			if ((ca->entity == NULL) && (!strcasecmp(ca->host, host)) && ((ca->level & level) == level))
				return ca;
		}
		else if ((ca->entity == NULL) && (!strcasecmp(ca->host, host)))
			return ca;
	}

	return NULL;
}

struct chanacs *
chanacs_find_host_by_user(struct mychan *mychan, struct user *u, unsigned int level)
{
	mowgli_node_t *n;
	struct chanacs *ca;

	return_val_if_fail(mychan != NULL && u != NULL, 0);

	for (n = next_matching_host_chanacs(mychan, u, mychan->chanacs.head); n != NULL; n = next_matching_host_chanacs(mychan, u, n->next))
	{
		ca = n->data;
		if ((ca->level & level) == level)
			return ca;
	}

	return NULL;
}

static unsigned int
chanacs_host_flags_by_user(struct mychan *mychan, struct user *u)
{
	mowgli_node_t *n;
	unsigned int result = 0;
	struct chanacs *ca;

	return_val_if_fail(mychan != NULL && u != NULL, 0);

	for (n = next_matching_host_chanacs(mychan, u, mychan->chanacs.head); n != NULL; n = next_matching_host_chanacs(mychan, u, n->next))
	{
		ca = n->data;
		result |= ca->level;
	}

	slog(LG_DEBUG, "chanacs_host_flags_by_user(%s, %s): return %s", mychan->name, u->nick, bitmask_to_flags(result));

	return result;
}

struct chanacs *
chanacs_find_by_mask(struct mychan *mychan, const char *mask, unsigned int level)
{
	struct myentity *mt;
	struct chanacs *ca;

	return_val_if_fail(mychan != NULL && mask != NULL, NULL);

	mt = myentity_find(mask);
	if (mt != NULL)
	{
		ca = chanacs_find_literal(mychan, mt, level);

		if (ca)
			return ca;
	}

	return chanacs_find_host_literal(mychan, mask, level);
}

bool
chanacs_user_has_flag(struct mychan *mychan, struct user *u, unsigned int level)
{
	struct myentity *mt;

	return_val_if_fail(mychan != NULL && u != NULL, false);

	mt = entity(u->myuser);
	if (mt != NULL)
	{
		if (chanacs_entity_has_flag(mychan, mt, level))
			return true;
	}

	if (chanacs_user_flags(mychan, u) & level)
		return true;

	return false;
}

static unsigned int
chanacs_entity_flags_by_user(struct mychan *mychan, struct user *u)
{
	mowgli_node_t *n;
	unsigned int result = 0;

	return_val_if_fail(mychan != NULL, 0);
	return_val_if_fail(u != NULL, 0);

	MOWGLI_ITER_FOREACH(n, mychan->chanacs.head)
	{
		struct chanacs *ca = n->data;
		struct myentity *mt;
		const struct entity_vtable *vt;

		if (ca->entity == NULL)
			continue;

		mt = ca->entity;
		vt = myentity_get_vtable(mt);

		if (vt->match_user && vt->match_user(mt, u))
			result |= ca->level;
	}

	slog(LG_DEBUG, "chanacs_entity_flags_by_user(%s, %s): return %s", mychan->name, u->nick, bitmask_to_flags(result));

	return result;
}

unsigned int
chanacs_user_flags(struct mychan *mychan, struct user *u)
{
	struct myentity *mt;
	unsigned int result = 0;

	return_val_if_fail(mychan != NULL && u != NULL, 0);

	mt = entity(u->myuser);
	if (mt != NULL)
		result |= chanacs_entity_flags(mychan, mt);

	result |= chanacs_entity_flags_by_user(mychan, u);

	/* the user is pending e-mail verification.  so, we want to filter out all flags
	 * other than CA_AKICK (+b).  that way they have no effective access.  --kaniini
	 */
	if (u->myuser != NULL && (u->myuser->flags & MU_WAITAUTH))
		result &= ~(ca_all & ~CA_AKICK);

	result |= chanacs_host_flags_by_user(mychan, u);

	slog(LG_DEBUG, "chanacs_user_flags(%s, %s): return %s", mychan->name, u->nick, bitmask_to_flags(result));

	return result;
}

unsigned int
chanacs_source_flags(struct mychan *mychan, struct sourceinfo *si)
{
	if (si->su != NULL)
	{
		return chanacs_user_flags(mychan, si->su);
	}
	else
	{
		if (entity(si->smu))
			return chanacs_entity_flags(mychan, entity(si->smu));
		else
			return 0;
	}
}

/* Look for the chanacs exactly matching mu or host (exactly one of mu and
 * host must be non-NULL). If not found, and create is true, create a new
 * chanacs with no flags.
 */
struct chanacs *
chanacs_open(struct mychan *mychan, struct myentity *mt, const char *hostmask, bool create, struct myentity *setter)
{
	struct chanacs *ca;

	/* wrt the second assert: only one of mu or hostmask can be not-NULL --nenolod */
	return_val_if_fail(mychan != NULL, false);
	return_val_if_fail((mt != NULL && hostmask == NULL) || (mt == NULL && hostmask != NULL), false);

	if (mt != NULL)
	{
		ca = chanacs_find_literal(mychan, mt, 0);
		if (ca != NULL)
			return ca;
		else if (create)
			return chanacs_add(mychan, mt, 0, CURRTIME, setter);
	}
	else
	{
		ca = chanacs_find_host_literal(mychan, hostmask, 0);
		if (ca != NULL)
			return ca;
		else if (create)
			return chanacs_add_host(mychan, hostmask, 0, CURRTIME, setter);
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
bool
chanacs_modify(struct chanacs *ca, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags, struct myuser *setter)
{
	return_val_if_fail(ca != NULL, false);
	return_val_if_fail(addflags != NULL && removeflags != NULL, false);

	*addflags &= ~ca->level;
	*removeflags &= ca->level & ~*addflags;
	/* no change? */
	if ((*addflags | *removeflags) == 0)
		return true;
	/* attempting to add bad flag? */
	if (~restrictflags & *addflags)
		return false;
	/* attempting to remove bad flag? */
	if (~restrictflags & *removeflags)
		return false;
	/* attempting to manipulate user with more privs? */
	if (~restrictflags & ca->level)
		return false;
	ca->level = (ca->level | *addflags) & ~*removeflags;
	ca->tmodified = CURRTIME;
	if (setter != NULL)
		mowgli_strlcpy(ca->setter_uid, entity(setter)->id, sizeof ca->setter_uid);
	else
		ca->setter_uid[0] = '\0';

	return true;
}

/* version that doesn't return the changes made */
bool
chanacs_modify_simple(struct chanacs *ca, unsigned int addflags, unsigned int removeflags, struct myuser *setter)
{
	unsigned int a, r;

	a = addflags & ca_all;
	r = removeflags & ca_all;
	return chanacs_modify(ca, &a, &r, ca_all, setter);
}

/* Change channel access
 *
 * Either mu or hostmask must be specified.
 * Add the flags in *addflags and remove the flags in *removeflags, updating
 * these to reflect the actual change. Only allow changes to restrictflags.
 * Returns true if successful, false if an unallowed change was attempted.
 * -- jilles */
bool
chanacs_change(struct mychan *mychan, struct myentity *mt, const char *hostmask, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags, struct myentity *setter)
{
	struct chanacs *ca;
	struct hook_channel_acl_req req;

	/* wrt the second assert: only one of mu or hostmask can be not-NULL --nenolod */
	return_val_if_fail(mychan != NULL, false);
	return_val_if_fail((mt != NULL && hostmask == NULL) || (mt == NULL && hostmask != NULL), false);
	return_val_if_fail(addflags != NULL && removeflags != NULL, false);

	if (mt != NULL)
	{
		ca = chanacs_find_literal(mychan, mt, 0);
		if (ca == NULL)
		{
			*removeflags = 0;
			/* no change? */
			if ((*addflags | *removeflags) == 0)
				return true;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return false;
			chanacs_add(mychan, mt, *addflags, CURRTIME, setter);
		}
		else
		{
			*addflags &= ~ca->level;
			*removeflags &= ca->level & ~*addflags;
			/* no change? */
			if ((*addflags | *removeflags) == 0)
				return true;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return false;
			/* attempting to remove bad flag? */
			if (~restrictflags & *removeflags)
				return false;
			/* attempting to manipulate user with more privs? */
			if (~restrictflags & ca->level)
				return false;
			ca->level = (ca->level | *addflags) & ~*removeflags;
			ca->tmodified = CURRTIME;
			if (setter != NULL)
				mowgli_strlcpy(ca->setter_uid, setter->id, sizeof ca->setter_uid);
			else
				ca->setter_uid[0] = '\0';

			if (ca->level == 0)
				atheme_object_unref(ca);
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
				return true;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return false;
			chanacs_add_host(mychan, hostmask, *addflags, CURRTIME, setter);
		}
		else
		{
			*addflags &= ~ca->level;
			*removeflags &= ca->level & ~*addflags;
			/* no change? */
			if ((*addflags | *removeflags) == 0)
				return true;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return false;
			/* attempting to remove bad flag? */
			if (~restrictflags & *removeflags)
				return false;
			/* attempting to manipulate user with more privs? */
			if (~restrictflags & ca->level)
				return false;
			ca->level = (ca->level | *addflags) & ~*removeflags;
			ca->tmodified = CURRTIME;
			if (setter != NULL)
				mowgli_strlcpy(ca->setter_uid, setter->id, sizeof ca->setter_uid);
			else
				ca->setter_uid[0] = '\0';

			if (ca->level == 0)
				atheme_object_unref(ca);
		}
	}
	return true;
}

/* version that doesn't return the changes made */
bool
chanacs_change_simple(struct mychan *mychan, struct myentity *mt, const char *hostmask, unsigned int addflags, unsigned int removeflags, struct myentity *setter)
{
	unsigned int a, r;

	a = addflags & ca_all;
	r = removeflags & ca_all;
	return chanacs_change(mychan, mt, hostmask, &a, &r, ca_all, setter);
}

static int
expire_myuser_cb(struct myentity *const restrict mt, void ATHEME_VATTR_UNUSED *const restrict unused)
{
	return_val_if_fail(isuser(mt), 0);

	struct myuser *const mu = user(mt);

	if (mu->flags & MU_HOLD)
		return 0;

	/* Don't expire accounts with privs on them in atheme.conf,
	 * otherwise someone can reregister them and take the privs.
	 *   -- jilles
	 */
	if (is_conf_soper(mu))
		return 0;

	// If they're logged in, update lastlogin time.  -- jilles
	if (MOWGLI_LIST_LENGTH(&mu->logins))
		mu->lastlogin = CURRTIME;

	/* If they're unverified, expire them after a day. Otherwise, expire them
	 * if expiration is enabled, and they have not logged in for that long.
	 *   -- amdj
	 */
	const bool uexpired = ((mu->flags & MU_WAITAUTH) && ((CURRTIME - mu->registered) >= SECONDS_PER_DAY));
	const bool vexpired = ((nicksvs.expiry > 0) && ((unsigned int)(CURRTIME - mu->lastlogin) >= nicksvs.expiry));
	const bool expired = uexpired || vexpired;

	struct hook_expiry_req req = {
		.data.mu    = mu,
		.do_expire  = expired,
	};

	(void) hook_call_user_check_expire(&req);

	// Don't let a hook prevent expiry of unverified accounts
	if (uexpired || req.do_expire)
	{
		(void) slog(LG_REGISTER, "EXPIRE:%s: \2%s\2 from \2%s\2",
		                         expired ? "CORE" : "HOOK", entity(mu)->name, mu->email);

		(void) slog(LG_VERBOSE, "expire_check(): %s expiring account %s (unused %us, email %s, logins %zu, "
		                        "nicks %zu, chanacs %zu)", expired ? "core" : "hook", entity(mu)->name,
		                        (unsigned int)(CURRTIME - mu->lastlogin), mu->email,
		                        MOWGLI_LIST_LENGTH(&mu->logins), MOWGLI_LIST_LENGTH(&mu->nicks),
		                        MOWGLI_LIST_LENGTH(&entity(mu)->chanacs));

		/* If they are logged in during expiration, the destructor
		 * for this object will take care of logging them out.
		 *   -- amdj
		 */
		(void) atheme_object_dispose(mu);
	}

	return 0;
}

void
expire_check(void *arg)
{
	struct mynick *mn;
	struct mychan *mc;
	struct user *u;
	mowgli_patricia_iteration_state_t state;
	struct hook_expiry_req req;

	/* Let them know about this and the likely subsequent db_save()
	 * right away -- jilles */
	if (curr_uplink != NULL && curr_uplink->conn != NULL)
		sendq_flush(curr_uplink->conn);

	myentity_foreach_t(ENT_USER, &expire_myuser_cb, NULL);

	MOWGLI_PATRICIA_FOREACH(mn, &state, nicklist)
	{
		req.do_expire = 1;
		req.data.mn = mn;

		hook_call_nick_check_expire(&req);

		if (!req.do_expire)
			continue;

		if (nicksvs.expiry > 0 && mn->lastseen < CURRTIME &&
				(unsigned int)(CURRTIME - mn->lastseen) >= nicksvs.expiry)
		{
			if (MU_HOLD & mn->owner->flags)
				continue;

			/* do not drop main nick like this */
			if (!irccasecmp(mn->nick, entity(mn->owner)->name))
				continue;

			u = user_find_named(mn->nick);
			if (u != NULL && u->myuser == mn->owner)
			{
				/* still logged in, bleh */
				mn->lastseen = CURRTIME;
				mn->owner->lastlogin = CURRTIME;
				continue;
			}

			slog(LG_REGISTER, "EXPIRE: \2%s\2 from \2%s\2", mn->nick, entity(mn->owner)->name);
			slog(LG_VERBOSE, "expire_check(): expiring nick %s (unused %lds, account %s)",
					mn->nick, (long)(CURRTIME - mn->lastseen),
					entity(mn->owner)->name);
			atheme_object_unref(mn);
		}
	}

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		req.do_expire = 1;
		req.data.mc = mc;

		hook_call_channel_check_expire(&req);

		if (!req.do_expire)
			continue;

		if ((unsigned int) (CURRTIME - mc->used) >= (SECONDS_PER_DAY - SECONDS_PER_HOUR - SECONDS_PER_MINUTE))
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

		if (chansvs.expiry > 0 && mc->used < CURRTIME &&
				(unsigned int)(CURRTIME - mc->used) >= chansvs.expiry)
		{
			if (MC_HOLD & mc->flags)
				continue;

			slog(LG_REGISTER, "EXPIRE: \2%s\2 from \2%s\2", mc->name, mychan_founder_names(mc));
			slog(LG_VERBOSE, "expire_check(): expiring channel %s (unused %lds, founder %s, chanacs %zu)",
					mc->name, (long)(CURRTIME - mc->used),
					mychan_founder_names(mc),
					MOWGLI_LIST_LENGTH(&mc->chanacs));

			hook_call_channel_drop(mc);
			if (mc->chan != NULL && !(mc->chan->flags & CHAN_LOG))
				part(mc->name, chansvs.nick);

			atheme_object_unref(mc);
		}
	}
}

static int
check_myuser_cb(struct myentity *mt, void *unused)
{
	struct myuser *mu = user(mt);
	mowgli_node_t *n;
	struct mynick *mn, *mn1;

	return_val_if_fail(isuser(mt), 0);

	if (!nicksvs.no_nick_ownership)
	{
		mn1 = NULL;
		MOWGLI_ITER_FOREACH(n, mu->nicks.head)
		{
			mn = n->data;
			if (mn->registered < mu->registered)
				mu->registered = mn->registered;
			if (mn->lastseen > mu->lastlogin)
				mu->lastlogin = mn->lastseen;
			if (!irccasecmp(entity(mu)->name, mn->nick))
				mn1 = mn;
		}
		mn = mn1 != NULL ? mn1 : mynick_find(entity(mu)->name);
		if (mn == NULL)
		{
			slog(LG_REGISTER, "db_check(): adding missing nick %s", entity(mu)->name);
			mn = mynick_add(mu, entity(mu)->name);
			mn->registered = mu->registered;
			mn->lastseen = mu->lastlogin;
		}
		else if (mn->owner != mu)
		{
			slog(LG_REGISTER, "db_check(): replacing nick %s owned by %s with %s", mn->nick, entity(mn->owner)->name, entity(mu)->name);
			atheme_object_unref(mn);
			mn = mynick_add(mu, entity(mu)->name);
			mn->registered = mu->registered;
			mn->lastseen = mu->lastlogin;
		}
	}

	return 0;
}

bool
user_loginmaxed(struct myuser *mu)
{
	if (mu->flags & MU_LOGINNOLIMIT)
		return false;
	else if (has_priv_myuser(mu, PRIV_LOGIN_NOLIMIT))
		return false;
	else if (MOWGLI_LIST_LENGTH(&mu->logins) < me.maxlogins)
		return false;
	else
		return true;
}

void
db_check(void)
{
	myentity_foreach_t(ENT_USER, check_myuser_cb, NULL);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
