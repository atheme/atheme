/*
 * Copyright (c) 2006 Atheme developers
 * Rights to this code are as documented in doc/LICENSE.
 *
 * User management functions.
 *
 * $Id: users.c 6859 2006-10-22 13:49:42Z jilles $
 */

#include "atheme.h"

static BlockHeap *user_heap;

dictionary_tree_t *userlist;
dictionary_tree_t *uidlist;

/*
 * init_users()
 *
 * Initializes the users heap and DTree.
 *
 * Inputs:
 *     - none
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - the users heap and DTree are initialized.
 */
void init_users(void)
{
	user_heap = BlockHeapCreate(sizeof(user_t), HEAP_USER);

	if (user_heap == NULL)
	{
		slog(LG_DEBUG, "init_users(): block allocator failure.");
		exit(EXIT_FAILURE);
	}

	userlist = dictionary_create("user", HASH_USER, irccasecmp);
	uidlist = dictionary_create("uid", HASH_USER, strcmp);
}

/*
 * user_add(const char *nick, const char *user, const char *host, const char *vhost, const char *ip,
 *          const char *uid, const char *gecos, server_t *server, uint32_t ts);
 *
 * User object factory.
 *
 * Inputs:
 *     - nickname of new user
 *     - username of new user
 *     - hostname of new user
 *     - virtual hostname of new user if applicable otherwise NULL
 *     - ip of user if applicable otherwise NULL
 *     - unique identifier (UID) of user if appliable otherwise NULL
 *     - gecos of new user
 *     - pointer to server new user is on
 *     - user's timestamp (XXX: this should be time_t, as timestamps actually ARE signed -- see POSIX)
 *
 * Outputs:
 *     - on success, a new user
 *     - on failure, NULL
 *
 * Side Effects:
 *     - if successful, a user is created and added to the users DTree.
 *
 * Bugs:
 *     - user timestamps should be time_t, not uint32_t.
 *     - this function does not check if a user object by this name already exists
 */
user_t *user_add(const char *nick, const char *user, const char *host, const char *vhost, const char *ip, const char *uid, const char *gecos, server_t *server, uint32_t ts)
{
	user_t *u;

	slog(LG_DEBUG, "user_add(): %s (%s@%s) -> %s", nick, user, host, server->name);

	u = BlockHeapAlloc(user_heap);

	if (uid != NULL)
	{
		strlcpy(u->uid, uid, IDLEN);
		dictionary_add(uidlist, u->uid, u);
	}

	strlcpy(u->nick, nick, NICKLEN);
	strlcpy(u->user, user, USERLEN);
	strlcpy(u->host, host, HOSTLEN);
	strlcpy(u->gecos, gecos, GECOSLEN);

	if (vhost)
		strlcpy(u->vhost, vhost, HOSTLEN);
	else
		strlcpy(u->vhost, host, HOSTLEN);

	if (ip && strcmp(ip, "0") && strcmp(ip, "0.0.0.0") && strcmp(ip, "255.255.255.255"))
		strlcpy(u->ip, ip, HOSTIPLEN);

	u->server = server;
	u->server->users++;
	node_add(u, node_create(), &u->server->userlist);

	if (ts)
		u->ts = ts;
	else
		u->ts = CURRTIME;

	dictionary_add(userlist, u->nick, u);

	cnt.user++;

	hook_call_event("user_add", u);

	return u;
}

/*
 * user_delete(user_t *u)
 *
 * Destroys a user object and deletes the object from the users DTree.
 *
 * Inputs:
 *     - user object to delete
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - on success, a user is deleted from the users DTree.
 */
void user_delete(user_t *u)
{
	node_t *n, *tn;
	chanuser_t *cu;

	if (!u)
	{
		slog(LG_DEBUG, "user_delete(): called for NULL user");
		return;
	}

	slog(LG_DEBUG, "user_delete(): removing user: %s -> %s", u->nick, u->server->name);

	hook_call_event("user_delete", u);

	u->server->users--;
	if (is_ircop(u))
		u->server->opers--;
	if (u->flags & UF_INVIS)
		u->server->invis--;

	/* remove the user from each channel */
	LIST_FOREACH_SAFE(n, tn, u->channels.head)
	{
		cu = (chanuser_t *)n->data;

		chanuser_delete(cu->chan, u);
	}

	dictionary_delete(userlist, u->nick);

	if (*u->uid)
		dictionary_delete(uidlist, u->uid);

	n = node_find(u, &u->server->userlist);
	node_del(n, &u->server->userlist);
	node_free(n);

	if (u->myuser)
	{
		LIST_FOREACH_SAFE(n, tn, u->myuser->logins.head)
		{
			if (n->data == u)
			{
				node_del(n, &u->myuser->logins);
				node_free(n);
				break;
			}
		}
		u->myuser->lastlogin = CURRTIME;
		u->myuser = NULL;
	}

	BlockHeapFree(user_heap, u);

	cnt.user--;
}

/*
 * user_find(const char *nick)
 *
 * Finds a user by UID or nickname.
 *
 * Inputs:
 *     - nickname or UID to look up
 *
 * Outputs:
 *     - on success, the user object that was requested
 *     - on failure, NULL
 *
 * Side Effects:
 *     - none
 */
user_t *user_find(const char *nick)
{
	user_t *u;

	if (nick == NULL)
		return NULL;

	if (ircd->uses_uid == TRUE)
	{
		u = dictionary_retrieve(uidlist, nick);

		if (u != NULL)
			return u;
	}

	u = dictionary_retrieve(userlist, nick);

	if (u != NULL)
	{
		if (ircd->uses_p10)
			wallops("user_find() found user %s by nick!", nick);
		return u;
	}

	return NULL;
}

/*
 * user_find_named(const char *nick)
 *
 * Finds a user by nickname. Prevents chasing users by their UID.
 *
 * Inputs:
 *     - nickname to look up
 *
 * Outputs:
 *     - on success, the user object that was requested
 *     - on failure, NULL
 *
 * Side Effects:
 *     - none
 */
user_t *user_find_named(const char *nick)
{
	return dictionary_retrieve(userlist, nick);
}

/*
 * user_changeuid(user_t *u, const char *uid)
 *
 * Changes a user object's UID.
 *
 * Inputs:
 *     - user object to change UID
 *     - new UID
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - a user object's UID is changed.
 */
void user_changeuid(user_t *u, const char *uid)
{
	node_t *n;

	if (*u->uid)
		dictionary_delete(uidlist, u->uid);

	strlcpy(u->uid, uid ? uid : "", IDLEN);

	if (*u->uid)
		dictionary_add(uidlist, u->uid, u);
}

/*
 * user_changenick(user_t *u, const char *uid)
 *
 * Changes a user object's nick and TS.
 *
 * Inputs:
 *     - user object to change
 *     - new nick
 *     - new TS
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - a user object's nick and TS is changed.
 */
void user_changenick(user_t *u, const char *nick, uint32_t ts)
{
	node_t *n;

	dictionary_delete(userlist, u->nick);

	strlcpy(u->nick, nick, NICKLEN);
	u->ts = ts;

	dictionary_add(userlist, u->nick, u);
}

/*
 * user_mode(user_t *user, char *modes)
 *
 * Changes a user object's modes.
 *
 * Inputs:
 *     - user object to change modes on
 *     - modestring describing the usermode change
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - on success, a user's modes are adjusted.
 *
 * Bugs:
 *     - this routine only tracks +i and +o usermode changes.
 */
void user_mode(user_t *user, char *modes)
{
	int dir = MTYPE_ADD;

	if (!user)
	{
		slog(LG_DEBUG, "user_mode(): called for nonexistant user");
		return;
	}

	while (*modes != '\0')
	{
		switch (*modes)
		{
		  case '+':
			  dir = MTYPE_ADD;
			  break;
		  case '-':
			  dir = MTYPE_DEL;
			  break;
		  case 'i':
			  if (dir == MTYPE_ADD)
			  {
				  if (!(user->flags & UF_INVIS))
					  user->server->invis++;
				  user->flags |= UF_INVIS;
			  }
			  else if ((dir = MTYPE_DEL))
			  {
				  if (user->flags & UF_INVIS)
					  user->server->invis--;
				  user->flags &= ~UF_INVIS;
			  }
			  break;
		  case 'o':
			  if (dir == MTYPE_ADD)
			  {
				  if (!is_ircop(user))
				  {
					  user->flags |= UF_IRCOP;
					  slog(LG_DEBUG, "user_mode(): %s is now an IRCop", user->nick);
					  snoop("OPER: %s (%s)", user->nick, user->server->name);
					  user->server->opers++;
					  hook_call_event("user_oper", user);
				  }
			  }
			  else if ((dir = MTYPE_DEL))
			  {
				  if (is_ircop(user))
				  {
					  user->flags &= ~UF_IRCOP;
					  slog(LG_DEBUG, "user_mode(): %s is no longer an IRCop", user->nick);
					  snoop("DEOPER: %s (%s)", user->nick, user->server->name);
					  user->server->opers--;
					  hook_call_event("user_deoper", user);
				  }
			  }
		  default:
			  break;
		}
		modes++;
	}
}
