/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Remote authentication cookie handling. (think kerberos.)
 *
 * $Id: authcookie.c 3847 2005-11-11 12:39:20Z jilles $
 */

#include "atheme.h"

list_t authcookie_list;
static BlockHeap *authcookie_heap;

void authcookie_init(void)
{
	authcookie_heap = BlockHeapCreate(sizeof(authcookie_t), 1024);

	if (!authcookie_heap)
	{
		slog(LG_ERROR, "authcookie_init(): cannot initialize block allocator.");
		exit(EXIT_FAILURE);
	}
}

/*
 * authcookie_create()
 *
 * Inputs:
 *       account associated with the authcookie
 *
 * Outputs:
 *       pointer to new authcookie
 *
 * Side Effects:
 *       an authcookie ticket is created, and validated.
 */
authcookie_t *authcookie_create(myuser_t *mu)
{
	authcookie_t *au = BlockHeapAlloc(authcookie_heap);

	au->ticket = gen_pw(20);
	au->myuser = mu;
	au->expire = CURRTIME + 3600;

	node_add(au, &au->node, &authcookie_list);

	return au;
}

/*
 * authcookie_find()
 *
 * Inputs:
 *       either the ticket string, the myuser_t it is associated with, or both
 *
 * Outputs:
 *       the authcookie ticket for this object, if any
 *
 * Side Effects:
 *       none
 */
authcookie_t *authcookie_find(char *ticket, myuser_t *myuser)
{
	node_t *n;
	authcookie_t *ac;

	/* at least one must be specified */
	if (!ticket && !myuser)
		return NULL;

	if (!myuser)		/* must have ticket */
	{
		LIST_FOREACH(n, authcookie_list.head)
		{
			ac = n->data;

			if (!strcmp(ac->ticket, ticket))
				return ac;
		}
	}
	else if (!ticket)	/* must have myuser */
	{
		LIST_FOREACH(n, authcookie_list.head)
		{
			ac = n->data;

			if (ac->myuser == myuser)
				return ac;
		}
	}
	else			/* must have both */
	{
		LIST_FOREACH(n, authcookie_list.head)
		{
			ac = n->data;

			if (ac->myuser == myuser
				&& !strcmp(ac->ticket, ticket))
				return ac;
		}
	}

	return NULL;
}

/*
 * authcookie_destroy()
 *
 * Inputs:
 *       an authcookie to destroy
 *
 * Outputs:
 *       none
 *
 * Side Effects:
 *       an authcookie is destroyed
 */
void authcookie_destroy(authcookie_t * ac)
{
	node_del(&ac->node, &authcookie_list);
	free(ac->ticket);
	BlockHeapFree(authcookie_heap, ac);
}

/*
 * authcookie_destroy_all()
 *
 * Inputs:
 *       a myuser_t pointer
 *
 * Outputs:
 *       none
 *
 * Side Effects:
 *       all authcookies for the user are destroyed
 */
void authcookie_destroy_all(myuser_t *mu)
{
	node_t *n, *tn;
	authcookie_t *ac;

	LIST_FOREACH_SAFE(n, tn, authcookie_list.head)
	{
		ac = n->data;

		if (ac->myuser == mu)
			authcookie_destroy(ac);
	}
}

/*
 * authcookie_expire()
 *
 * Inputs:
 *       unused arg because this is an event function
 *
 * Outputs:
 *       none
 *
 * Side Effects:
 *       expired authcookies are destroyed
 */
void authcookie_expire(void *arg)
{
	authcookie_t *ac;
	node_t *n, *tn;

	(void)arg;
        LIST_FOREACH_SAFE(n, tn, authcookie_list.head)
	{
		ac = n->data;

		if (ac->expire <= CURRTIME)
			authcookie_destroy(ac);
	}
}

/*
 * authcookie_validate()
 *
 * Inputs:
 *       a ticket and myuser pair that needs to be validated
 *
 * Outputs:
 *       TRUE if the authcookie is valid,
 *       otherwise FALSE
 *
 * Side Effects:
 *       expired authcookies are destroyed here
 */
boolean_t authcookie_validate(char *ticket, myuser_t *myuser)
{
	authcookie_t *ac = authcookie_find(ticket, myuser);

	if (!ac)
		return FALSE;

	if (ac->expire <= CURRTIME)
	{
		authcookie_destroy(ac);
		return FALSE;
	}

	return TRUE;
}
