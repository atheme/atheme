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

mowgli_patricia_t *nicklist;
mowgli_patricia_t *oldnameslist;
mowgli_patricia_t *mclist;
mowgli_patricia_t *certfplist;

mowgli_heap_t *myuser_heap;   /* HEAP_USER */
mowgli_heap_t *mynick_heap;   /* HEAP_USER */
mowgli_heap_t *mycertfp_heap; /* HEAP_USER */
mowgli_heap_t *myuser_name_heap;	/* HEAP_USER / 2 */
mowgli_heap_t *mychan_heap;	/* HEAP_CHANNEL */
mowgli_heap_t *chanacs_heap;	/* HEAP_CHANACS */

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
	myuser_heap = mowgli_heap_create(sizeof(myuser_t), HEAP_USER, BH_NOW);
	mynick_heap = mowgli_heap_create(sizeof(myuser_t), HEAP_USER, BH_NOW);
	myuser_name_heap = mowgli_heap_create(sizeof(myuser_name_t), HEAP_USER / 2, BH_NOW);
	mychan_heap = mowgli_heap_create(sizeof(mychan_t), HEAP_CHANNEL, BH_NOW);
	chanacs_heap = mowgli_heap_create(sizeof(chanacs_t), HEAP_CHANUSER, BH_NOW);
	mycertfp_heap = mowgli_heap_create(sizeof(mycertfp_t), HEAP_USER, BH_NOW);

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
myuser_t *myuser_add(const char *name, const char *pass, const char *email, unsigned int flags)
{
	myuser_t *mu;
	soper_t *soper;

	return_val_if_fail((mu = myuser_find(name)) == NULL, mu);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "myuser_add(): %s -> %s", name, email);

	mu = mowgli_heap_alloc(myuser_heap);
	object_init(object(mu), NULL, (destructor_t) myuser_delete);

	entity(mu)->type = ENT_USER;
	strlcpy(entity(mu)->name, name, NICKLEN);
	mu->email = sstrdup(email);

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
		strlcpy(mu->pass, pass, PASSLEN);
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
	user_t *u;
	mowgli_node_t *n, *tn;
	mymemo_t *memo;
	chanacs_t *ca;
	char nicks[200];

	return_if_fail(mu != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "myuser_delete(): %s", entity(mu)->name);

	myuser_name_remember(entity(mu)->name, mu);

	hook_call_myuser_delete(mu);

	/* log them out */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->logins.head)
	{
		u = (user_t *)n->data;
		if (!authservice_loaded || !ircd_on_logout(u, entity(mu)->name))
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
			slog(LG_INFO, _("SUCCESSION: \2%s\2 to \2%s\2 from \2%s\2"), mc->name, entity(successor)->name, entity(mu)->name);
			slog(LG_VERBOSE, "myuser_delete(): giving channel %s to %s (unused %lds, founder %s, chanacs %zu)",
					mc->name, entity(successor)->name,
					(long)(CURRTIME - mc->used),
					entity(mu)->name,
					MOWGLI_LIST_LENGTH(&mc->chanacs));
			if (chansvs.me != NULL)
				verbose(mc, "Foundership changed to \2%s\2 because \2%s\2 was dropped.", entity(successor)->name, entity(mu)->name);

			chanacs_change_simple(mc, entity(successor), NULL, CA_FOUNDER_0, 0);
			if (chansvs.me != NULL)
				myuser_notice(chansvs.nick, successor, "You are now founder on \2%s\2 (as \2%s\2).", mc->name, entity(successor)->name);
			object_unref(ca);
		}
		/* no successor found */
		else if (ca->level & CA_FOUNDER && mychan_num_founders(mc) == 1)
		{
			slog(LG_REGISTER, _("DELETE: \2%s\2 from \2%s\2"), mc->name, entity(mu)->name);
			slog(LG_VERBOSE, "myuser_delete(): deleting channel %s (unused %lds, founder %s, chanacs %zu)",
					mc->name, (long)(CURRTIME - mc->used),
					entity(mu)->name,
					MOWGLI_LIST_LENGTH(&mc->chanacs));

			hook_call_channel_drop(mc);
			if (mc->chan != NULL && (config_options.chan == NULL || irccasecmp(mc->name, config_options.chan)) && !(mc->chan->flags & CHAN_LOG))
				part(mc->name, chansvs.nick);
			object_unref(mc);
		}
		else /* not founder */
			object_unref(ca);
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
		memo = (mymemo_t *)n->data;

		mowgli_node_delete(n, &mu->memos);
		mowgli_node_free(n);
		free(memo);
	}

	/* delete access entries */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->access_list.head)
		myuser_access_delete(mu, (char *)n->data);

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
				slog(LG_REGISTER, _("DELETE: \2%s\2 from \2%s\2"), nicks, entity(mu)->name);
				nicks[0] = '\0';
			}
			if (nicks[0] != '\0')
				strlcat(nicks, ", ", sizeof nicks);
			strlcat(nicks, "\2", sizeof nicks);
			strlcat(nicks, mn->nick, sizeof nicks);
			strlcat(nicks, "\2", sizeof nicks);
		}
		object_unref(mn);
	}
	if (nicks[0] != '\0')
		slog(LG_REGISTER, _("DELETE: \2%s\2 from \2%s\2"), nicks, entity(mu)->name);

	/* entity(mu)->name is the index for this dtree */
	myentity_del(entity(mu));

	free(mu->email);

	mowgli_heap_free(myuser_heap, mu);

	cnt.myuser--;
}

/*
 * myuser_rename(myuser_t *mu, const char *name)
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
void myuser_rename(myuser_t *mu, const char *name)
{
	mowgli_node_t *n, *tn;
	user_t *u;
	hook_user_rename_t data;

	char nb[NICKLEN];
	strlcpy(nb, entity(mu)->name, NICKLEN);

	if (authservice_loaded)
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->logins.head)
		{
			u = n->data;
			ircd_on_logout(u, entity(mu)->name);
		}
	}
	myentity_del(entity(mu));
	strlcpy(entity(mu)->name, name, NICKLEN);
	myentity_put(entity(mu));
	if (authservice_loaded)
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
 * myuser_set_email(myuser_t *mu, const char *name)
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
void myuser_set_email(myuser_t *mu, const char *newemail)
{
	free(mu->email);
	mu->email = sstrdup(newemail);
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
 * myuser_notice(const char *from, myuser_t *target, const char *fmt, ...)
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
void myuser_notice(const char *from, myuser_t *target, const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	mowgli_node_t *n;
	user_t *u;

	if (target == NULL)
		return;

	/* have to reformat it here, can't give a va_list to notice() :(
	 * -- jilles
	 */
	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	MOWGLI_ITER_FOREACH(n, target->logins.head)
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
 *     - true if user matches an accesslist entry
 *     - false otherwise
 *
 * Side Effects:
 *     - none
 */
bool
myuser_access_verify(user_t *u, myuser_t *mu)
{
	mowgli_node_t *n;
	char buf[USERLEN+HOSTLEN];
	char buf2[USERLEN+HOSTLEN];
	char buf3[USERLEN+HOSTLEN];
	char buf4[USERLEN+HOSTLEN];

	if (u == NULL || mu == NULL)
	{
		slog(LG_DEBUG, "myuser_access_verify(): invalid parameters: u = %p, mu = %p", u, mu);
		return false;
	}

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
myuser_access_add(myuser_t *mu, const char *mask)
{
	mowgli_node_t *n;
	char *msk;

	if (mu == NULL || mask == NULL)
	{
		slog(LG_DEBUG, "myuser_access_add(): invalid parameters: mu = %p, mask = %p", mu, mask);
		return false;
	}

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
myuser_access_find(myuser_t *mu, const char *mask)
{
	mowgli_node_t *n;

	if (mu == NULL || mask == NULL)
	{
		slog(LG_DEBUG, "myuser_access_find(): invalid parameters: mu = %p, mask = %p", mu, mask);
		return NULL;
	}

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
myuser_access_delete(myuser_t *mu, const char *mask)
{
	mowgli_node_t *n, *tn;

	if (mu == NULL || mask == NULL)
	{
		slog(LG_DEBUG, "myuser_access_delete(): invalid parameters: mu = %p, mask = %p", mu, mask);
		return;
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, mu->access_list.head)
	{
		char *entry = (char *) n->data;

		if (!strcasecmp(entry, mask))
		{
			mowgli_node_delete(n, &mu->access_list);
			mowgli_node_free(n);
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
		slog(LG_DEBUG, "mynick_add(): %s -> %s", name, entity(mu)->name);

	mn = mowgli_heap_alloc(mynick_heap);
	object_init(object(mn), NULL, (destructor_t) mynick_delete);

	strlcpy(mn->nick, name, NICKLEN);
	mn->owner = mu;
	mn->registered = CURRTIME;

	mowgli_patricia_add(nicklist, mn->nick, mn);
	mowgli_node_add(mn, &mn->node, &mu->nicks);

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
	mowgli_node_delete(&mn->node, &mn->owner->nicks);

	mowgli_heap_free(mynick_heap, mn);

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

	mun = mowgli_heap_alloc(myuser_name_heap);
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
	return_if_fail(mun != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "myuser_name_delete(): %s", mun->name);

	mowgli_patricia_delete(oldnameslist, mun->name);

	metadata_delete_all(mun);

	mowgli_heap_free(myuser_name_heap, mun);

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
	mowgli_node_t *n;
	char *copy;

	mun = myuser_name_find(name);
	if (mun == NULL)
		return;

	md = metadata_find(mu, "private:mark:reason");
	md2 = metadata_find(mun, "private:mark:reason");
	if (md != NULL && md2 != NULL && strcmp(md->value, md2->value))
	{
		wallops(_("Not restoring mark \2\"%s\"\2 for account \2%s\2 (name \2%s\2) which is already marked"), md2->value, entity(mu)->name, name);
		slog(LG_INFO, _("MARK:FORGET: \2\"%s\"\2 for \2%s (%s)\2 (already marked)"), md2->value, name, entity(mu)->name);
		slog(LG_VERBOSE, "myuser_name_restore(): not restoring mark \"%s\" for account %s (name %s) which is already marked",
				md2->value, entity(mu)->name, name);
	}
	else if (md == NULL && md2 != NULL)
	{
		slog(LG_INFO, _("MARK:RESTORE: \2\"%s\"\2 for \2%s (%s)\2"), md2->value, name, entity(mu)->name);
		slog(LG_VERBOSE, "myuser_name_restore(): restoring mark \"%s\" for account %s (name %s)",
				md2->value, entity(mu)->name, name);
	}

	MOWGLI_ITER_FOREACH(n, object(mun)->metadata.head)
	{
		md = n->data;
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
				free(copy);
			}
		}
	}

	object_unref(mun);

	return;
}

/*******************
 * M Y C E R T F P *
 *******************/

mycertfp_t *mycertfp_add(myuser_t *mu, const char *certfp)
{
	mycertfp_t *mcfp;

	return_val_if_fail(mu != NULL, NULL);
	return_val_if_fail(certfp != NULL, NULL);

	mcfp = mowgli_heap_alloc(mycertfp_heap);
	mcfp->mu = mu;
	mcfp->certfp = sstrdup(certfp);

	mowgli_node_add(mcfp, &mcfp->node, &mu->cert_fingerprints);
	mowgli_patricia_add(certfplist, mcfp->certfp, mcfp);

	return mcfp;
}

void mycertfp_delete(mycertfp_t *mcfp)
{
	return_if_fail(mcfp != NULL);
	return_if_fail(mcfp->mu != NULL);
	return_if_fail(mcfp->certfp != NULL);

	mowgli_node_delete(&mcfp->node, &mcfp->mu->cert_fingerprints);
	mowgli_patricia_delete(certfplist, mcfp->certfp);

	free(mcfp->certfp);
	mowgli_heap_free(mycertfp_heap, mcfp);
}

mycertfp_t *mycertfp_find(const char *certfp)
{
	return_val_if_fail(certfp != NULL, NULL);

	return mowgli_patricia_retrieve(certfplist, certfp);
}

/***************
 * M Y C H A N *
 ***************/

/* private destructor for mychan_t. */
static void mychan_delete(mychan_t *mc)
{
	mowgli_node_t *n, *tn;

	return_if_fail(mc != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "mychan_delete(): %s", mc->name);

	/* remove the chanacs shiz */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chanacs.head)
		object_unref(n->data);

	metadata_delete_all(mc);

	mowgli_patricia_delete(mclist, mc->name);

	free(mc->name);

	mowgli_heap_free(mychan_heap, mc);

	cnt.mychan--;
}

mychan_t *mychan_add(char *name)
{
	mychan_t *mc;

	return_val_if_fail((mc = mychan_find(name)) == NULL, mc);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "mychan_add(): %s", name);

	mc = mowgli_heap_alloc(mychan_heap);

	object_init(object(mc), NULL, (destructor_t) mychan_delete);
	mc->name = sstrdup(name);
	mc->registered = CURRTIME;
	mc->chan = channel_find(name);

	mowgli_patricia_add(mclist, mc->name, mc);

	cnt.mychan++;

	return mc;
}


/* Check if there is anyone on the channel fulfilling the conditions.
 * Fairly expensive, but this is sometimes necessary to avoid
 * inappropriate drops. -- jilles */
bool mychan_isused(mychan_t *mc)
{
	mowgli_node_t *n;
	channel_t *c;
	chanuser_t *cu;

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

unsigned int mychan_num_founders(mychan_t *mc)
{
	mowgli_node_t *n;
	chanacs_t *ca;
	unsigned int count = 0;

	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;
		if (ca->entity != NULL && ca->level & CA_FOUNDER)
			count++;
	}
	return count;
}

const char *mychan_founder_names(mychan_t *mc)
{
	mowgli_node_t *n;
	chanacs_t *ca;
	static char names[512];

	names[0] = '\0';
	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;
		if (ca->entity != NULL && ca->level & CA_FOUNDER)
		{
			if (names[0] != '\0')
				strlcat(names, ", ", sizeof names);
			strlcat(names, entity(ca->entity)->name, sizeof names);
		}
	}
	return names;
}

static unsigned int add_auto_flags(unsigned int flags)
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
#define RECENTLY_SEEN (7 * 86400)

/* Find a user fulfilling the conditions who can take another channel */
myuser_t *mychan_pick_candidate(mychan_t *mc, unsigned int minlevel)
{
	mowgli_node_t *n;
	chanacs_t *ca;
	myentity_t *mt, *hi_mt;
	unsigned int level, hi_level;
	bool recent_ok = false;
	bool hi_recent_ok = false;

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
myuser_t *mychan_pick_successor(mychan_t *mc)
{
	myuser_t *mu;
	hook_channel_succession_req_t req;

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

const char *mychan_get_mlock(mychan_t *mc)
{
	static char buf[BUFSIZE];
	char params[BUFSIZE];
	metadata_t *md;
	char *p, *q, *qq;
	int dir;

	*buf = 0;
	*params = 0;

	md = metadata_find(mc, "private:mlockext");
	if (md != NULL && strlen(md->value) > 450)
		md = NULL;

	dir = MTYPE_NUL;

	if (mc->mlock_on)
	{
		if (dir != MTYPE_ADD)
			dir = MTYPE_ADD, strcat(buf, "+");
		strcat(buf, flags_to_string(mc->mlock_on));
	}

	if (mc->mlock_limit)
	{
		if (dir != MTYPE_ADD)
			dir = MTYPE_ADD, strcat(buf, "+");
		strcat(buf, "l");
		strcat(params, " ");
		strcat(params, number_to_string(mc->mlock_limit));
	}

	if (mc->mlock_key)
	{
		if (dir != MTYPE_ADD)
			dir = MTYPE_ADD, strcat(buf, "+");
		strcat(buf, "k");
		strcat(params, " *");
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
					dir = MTYPE_ADD, *q++ = '+';
				*q++ = *p++;
				strcat(params, " ");
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
			dir = MTYPE_DEL, strcat(buf, "-");
		strcat(buf, flags_to_string(mc->mlock_off));
		if (mc->mlock_off & CMODE_LIMIT)
			strcat(buf, "l");
		if (mc->mlock_off & CMODE_KEY)
			strcat(buf, "k");
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
					dir = MTYPE_DEL, *q++ = '-';
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

	strlcat(buf, params, BUFSIZE);

	if (*buf == '\0')
		strlcpy(buf, "+", BUFSIZE);

	return buf;
}

const char *mychan_get_sts_mlock(mychan_t *mc)
{
	static char mlock[BUFSIZE];
	metadata_t *md;

	mlock[0] = '\0';

	if (mc->mlock_on)
		strlcat(mlock, flags_to_string(mc->mlock_on), sizeof(mlock));
	if (mc->mlock_off)
		strlcat(mlock, flags_to_string(mc->mlock_off), sizeof(mlock));
	if (mc->mlock_limit || mc->mlock_off & CMODE_LIMIT)
		strlcat(mlock, "l", sizeof(mlock));
	if (mc->mlock_key || mc->mlock_off & CMODE_KEY)
		strlcat(mlock, "k", sizeof(mlock));
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

/* private destructor for chanacs_t */
static void chanacs_delete(chanacs_t *ca)
{
	mowgli_node_t *n;

	return_if_fail(ca != NULL);
	return_if_fail(ca->mychan != NULL);

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "chanacs_delete(): %s -> %s", ca->mychan->name,
			ca->entity != NULL ? entity(ca->entity)->name : ca->host);
	mowgli_node_delete(&ca->cnode, &ca->mychan->chanacs);

	if (ca->entity != NULL)
	{
		n = mowgli_node_find(ca, &ca->entity->chanacs);
		mowgli_node_delete(n, &ca->entity->chanacs);
		mowgli_node_free(n);
	}

	metadata_delete_all(ca);

	free(ca->host);

	mowgli_heap_free(chanacs_heap, ca);

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
chanacs_t *chanacs_add(mychan_t *mychan, myentity_t *mt, unsigned int level, time_t ts)
{
	chanacs_t *ca;
	mowgli_node_t *n;

	return_val_if_fail(mychan != NULL && mt != NULL, NULL);

	if (*mychan->name != '#')
	{
		slog(LG_DEBUG, "chanacs_add(): got non #channel: %s", mychan->name);
		return NULL;
	}

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "chanacs_add(): %s -> %s", mychan->name, mt->name);

	n = mowgli_node_create();

	ca = mowgli_heap_alloc(chanacs_heap);

	object_init(object(ca), NULL, (destructor_t) chanacs_delete);
	ca->mychan = mychan;
	ca->entity = mt;
	ca->host = NULL;
	ca->level = level & ca_all;
	ca->tmodified = ts;

	mowgli_node_add(ca, &ca->cnode, &mychan->chanacs);
	mowgli_node_add(ca, n, &mt->chanacs);

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

	return_val_if_fail(mychan != NULL && host != NULL, NULL);

	if (*mychan->name != '#')
	{
		slog(LG_DEBUG, "chanacs_add_host(): got non #channel: %s", mychan->name);
		return NULL;
	}

	if (!(runflags & RF_STARTING))
		slog(LG_DEBUG, "chanacs_add_host(): %s -> %s", mychan->name, host);

	ca = mowgli_heap_alloc(chanacs_heap);

	object_init(object(ca), NULL, (destructor_t) chanacs_delete);
	ca->mychan = mychan;
	ca->entity = NULL;
	ca->host = sstrdup(host);
	ca->level = level & ca_all;
	ca->tmodified = ts;

	mowgli_node_add(ca, &ca->cnode, &mychan->chanacs);

	cnt.chanacs++;

	return ca;
}

chanacs_t *chanacs_find(mychan_t *mychan, myentity_t *mt, unsigned int level)
{
	mowgli_node_t *n;
	chanacs_t *ca;

	return_val_if_fail(mychan != NULL && mt != NULL, NULL);

	if ((ca = chanacs_find_literal(mychan, mt, level)) != NULL)
		return ca;

	MOWGLI_ITER_FOREACH(n, mychan->chanacs.head)
	{
		entity_chanacs_validation_vtable_t *vt;

		ca = (chanacs_t *)n->data;

		if (ca->entity == NULL)
			continue;

		vt = myentity_get_chanacs_validator(ca->entity);
		if (level != 0x0)
		{
			if ((vt->match_entity(ca, mt) != NULL) && ((ca->level & level) == level))
				return ca;
		}
		else if (vt->match_entity(ca, mt) != NULL)
			return ca;
	}

	return NULL;
}

chanacs_t *chanacs_find_literal(mychan_t *mychan, myentity_t *mt, unsigned int level)
{
	mowgli_node_t *n;
	chanacs_t *ca;

	return_val_if_fail(mychan != NULL && mt != NULL, NULL);

	MOWGLI_ITER_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

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

chanacs_t *chanacs_find_host(mychan_t *mychan, const char *host, unsigned int level)
{
	mowgli_node_t *n;
	chanacs_t *ca;

	return_val_if_fail(mychan != NULL && host != NULL, NULL);

	MOWGLI_ITER_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

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

unsigned int chanacs_host_flags(mychan_t *mychan, const char *host)
{
	mowgli_node_t *n;
	chanacs_t *ca;
	unsigned int result = 0;

	return_val_if_fail(mychan != NULL && host != NULL, 0);

	MOWGLI_ITER_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (ca->entity == NULL && !match(ca->host, host))
			result |= ca->level;
	}

	return result;
}

chanacs_t *chanacs_find_host_literal(mychan_t *mychan, const char *host, unsigned int level)
{
	mowgli_node_t *n;
	chanacs_t *ca;

	if ((!mychan) || (!host))
		return NULL;

	MOWGLI_ITER_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

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

chanacs_t *chanacs_find_host_by_user(mychan_t *mychan, user_t *u, unsigned int level)
{
	mowgli_node_t *n;
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
	mowgli_node_t *n;
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
	myentity_t *mt;
	chanacs_t *ca;

	return_val_if_fail(mychan != NULL && mask != NULL, NULL);

	mt = myentity_find(mask);
	if (mt != NULL)
	{
		ca = chanacs_find(mychan, mt, level);

		if (ca)
			return ca;
	}

	return chanacs_find_host_literal(mychan, mask, level);
}

bool chanacs_user_has_flag(mychan_t *mychan, user_t *u, unsigned int level)
{
	myentity_t *mt;

	return_val_if_fail(mychan != NULL && u != NULL, false);

	mt = entity(u->myuser);
	if (mt != NULL)
	{
		if (chanacs_find(mychan, mt, level))
			return true;
	}

	if (chanacs_find_host_by_user(mychan, u, level))
		return true;

	return false;
}

unsigned int chanacs_user_flags(mychan_t *mychan, user_t *u)
{
	myentity_t *mt;
	chanacs_t *ca;
	unsigned int result = 0;

	return_val_if_fail(mychan != NULL && u != NULL, 0);

	mt = entity(u->myuser);
	if (mt != NULL)
	{
		ca = chanacs_find(mychan, mt, 0);
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
		ca = chanacs_find(mychan, entity(si->smu), 0);
		return ca != NULL ? ca->level : 0;
	}
}

/* Look for the chanacs exactly matching mu or host (exactly one of mu and
 * host must be non-NULL). If not found, and create is true, create a new
 * chanacs with no flags.
 */
chanacs_t *chanacs_open(mychan_t *mychan, myentity_t *mt, const char *hostmask, bool create)
{
	chanacs_t *ca;

	/* wrt the second assert: only one of mu or hostmask can be not-NULL --nenolod */
	return_val_if_fail(mychan != NULL, false);
	return_val_if_fail((mt != NULL && hostmask == NULL) || (mt == NULL && hostmask != NULL), false); 

	if (mt != NULL)
	{
		ca = chanacs_find_literal(mychan, mt, 0);
		if (ca != NULL)
			return ca;
		else if (create)
			return chanacs_add(mychan, mt, 0, CURRTIME);
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
bool chanacs_modify(chanacs_t *ca, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags)
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

	return true;
}

/* version that doesn't return the changes made */
bool chanacs_modify_simple(chanacs_t *ca, unsigned int addflags, unsigned int removeflags)
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
bool chanacs_change(mychan_t *mychan, myentity_t *mt, const char *hostmask, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags)
{
	chanacs_t *ca;

	/* wrt the second assert: only one of mu or hostmask can be not-NULL --nenolod */
	return_val_if_fail(mychan != NULL, false);
	return_val_if_fail((mt != NULL && hostmask == NULL) || (mt == NULL && hostmask != NULL), false); 
	return_val_if_fail(addflags != NULL && removeflags != NULL, false);

	if (mt != NULL)
	{
		ca = chanacs_find(mychan, mt, 0);
		if (ca == NULL)
		{
			*removeflags = 0;
			/* no change? */
			if ((*addflags | *removeflags) == 0)
				return true;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return false;
			chanacs_add(mychan, mt, *addflags, CURRTIME);
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
				return true;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return false;
			chanacs_add_host(mychan, hostmask, *addflags, CURRTIME);
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
			if (ca->level == 0)
				object_unref(ca);
		}
	}
	return true;
}

/* version that doesn't return the changes made */
bool chanacs_change_simple(mychan_t *mychan, myentity_t *mt, const char *hostmask, unsigned int addflags, unsigned int removeflags)
{
	unsigned int a, r;

	a = addflags & ca_all;
	r = removeflags & ca_all;
	return chanacs_change(mychan, mt, hostmask, &a, &r, ca_all);
}

static int expire_myuser_cb(myentity_t *mt, void *unused)
{
	hook_expiry_req_t req;
	myuser_t *mu = user(mt);

	return_val_if_fail(isuser(mt), 0);

	/* If they're logged in, update lastlogin time.
	 * To decrease db traffic, may want to only do
	 * this if the account would otherwise be
	 * deleted. -- jilles 
	 */
	if (MOWGLI_LIST_LENGTH(&mu->logins) > 0)
	{
		mu->lastlogin = CURRTIME;
		return 0;
	}

	if (MU_HOLD & mu->flags)
		return 0;

	req.data.mu = mu;
	req.do_expire = 1;
	hook_call_user_check_expire(&req);

	if (!req.do_expire)
		return 0;

	if ((nicksvs.expiry > 0 && mu->lastlogin < CURRTIME && (unsigned int)(CURRTIME - mu->lastlogin) >= nicksvs.expiry) ||
			(mu->flags & MU_WAITAUTH && CURRTIME - mu->registered >= 86400))
	{
		/* Don't expire accounts with privs on them in atheme.conf,
		 * otherwise someone can reregister
		 * them and take the privs -- jilles */
		if (is_conf_soper(mu))
			return 0;

		slog(LG_REGISTER, _("EXPIRE: \2%s\2 from \2%s\2 "), entity(mu)->name, mu->email);
		slog(LG_VERBOSE, "expire_check(): expiring account %s (unused %ds, email %s, nicks %zu, chanacs %zu)",
				entity(mu)->name, (int)(CURRTIME - mu->lastlogin),
				mu->email, MOWGLI_LIST_LENGTH(&mu->nicks),
				MOWGLI_LIST_LENGTH(&entity(mu)->chanacs));
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
	hook_expiry_req_t req;

	/* Let them know about this and the likely subsequent db_save()
	 * right away -- jilles */
	sendq_flush(curr_uplink->conn);

	myentity_foreach_t(ENT_USER, expire_myuser_cb, NULL);

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

			slog(LG_REGISTER, _("EXPIRE: \2%s\2 from \2%s\2"), mn->nick, entity(mn->owner)->name);
			slog(LG_VERBOSE, "expire_check(): expiring nick %s (unused %lds, account %s)",
					mn->nick, (long)(CURRTIME - mn->lastseen),
					entity(mn->owner)->name);
			object_unref(mn);
		}
	}

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		req.do_expire = 1;
		req.data.mc = mc;

		hook_call_channel_check_expire(&req);

		if (!req.do_expire)
			continue;

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

		if (chansvs.expiry > 0 && mc->used < CURRTIME &&
				(unsigned int)(CURRTIME - mc->used) >= chansvs.expiry)
		{
			if (MC_HOLD & mc->flags)
				continue;

			slog(LG_REGISTER, _("EXPIRE: \2%s\2 from \2%s\2"), mc->name, mychan_founder_names(mc));
			slog(LG_VERBOSE, "expire_check(): expiring channel %s (unused %lds, founder %s, chanacs %zu)",
					mc->name, (long)(CURRTIME - mc->used),
					mychan_founder_names(mc),
					MOWGLI_LIST_LENGTH(&mc->chanacs));

			hook_call_channel_drop(mc);
			if (mc->chan != NULL && (config_options.chan == NULL || irccasecmp(mc->name, config_options.chan)) && !(mc->chan->flags & CHAN_LOG))
				part(mc->name, chansvs.nick);

			object_unref(mc);
		}
	}
}

static int check_myuser_cb(myentity_t *mt, void *unused)
{
	myuser_t *mu = user(mt);
	mowgli_node_t *n;
	mynick_t *mn, *mn1;

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
			object_unref(mn);
			mn = mynick_add(mu, entity(mu)->name);
			mn->registered = mu->registered;
			mn->lastseen = mu->lastlogin;
		}
	}

	return 0;
}

void db_check(void)
{
	myentity_foreach_t(ENT_USER, check_myuser_cb, NULL);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
