/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Remote authentication cookie handling. (think kerberos.)
 *
 * $Id: authcookie.c 3305 2005-10-31 00:19:14Z alambert $
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
 *       either the ticket string or the myuser_t it is associated with
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

	if (ticket != NULL)
	{
		LIST_FOREACH(n, authcookie_list.head)
		{
			ac = n->data;

			if (!strcmp(ac->ticket, ticket))
				return ac;
		}
	}
	else if (myuser != NULL)
	{
		LIST_FOREACH(n, authcookie_list.head)
		{
			ac = n->data;

			if (ac->myuser == myuser)
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
	authcookie_t *ac = authcookie_find(NULL, myuser);

	if (!strcmp(ticket, ac->ticket))
		return FALSE;

	if (ac->expire < CURRTIME)
	{
		authcookie_destroy(ac);
		return FALSE;
	}

	return TRUE;
}
