/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2011 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * authcookie.c: Remote authentication ticket management
 */

#include <atheme.h>
#include "internal.h"

static mowgli_list_t authcookie_list;
static mowgli_heap_t *authcookie_heap = NULL;

void
authcookie_init(void)
{
	authcookie_heap = sharedheap_get(sizeof(struct authcookie));

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
struct authcookie *
authcookie_create(struct myuser *mu)
{
	struct authcookie *const au = mowgli_heap_alloc(authcookie_heap);
	au->ticket = random_string(AUTHCOOKIE_LENGTH);
	au->myuser = mu;
	au->expire = CURRTIME + SECONDS_PER_HOUR;

	mowgli_node_add(au, &au->node, &authcookie_list);

	return au;
}

/*
 * authcookie_find()
 *
 * Inputs:
 *       either the ticket string, the struct myuser it is associated with, or both
 *
 * Outputs:
 *       the authcookie ticket for this object, if any
 *
 * Side Effects:
 *       none
 */
struct authcookie *
authcookie_find(const char *ticket, struct myuser *myuser)
{
	mowgli_node_t *n;
	struct authcookie *ac;

	/* at least one must be specified */
	return_val_if_fail(ticket != NULL || myuser != NULL, NULL);

	if (!myuser)		/* must have ticket */
	{
		MOWGLI_ITER_FOREACH(n, authcookie_list.head)
		{
			ac = n->data;

			if (!strcmp(ac->ticket, ticket))
				return ac;
		}
	}
	else if (!ticket)	/* must have myuser */
	{
		MOWGLI_ITER_FOREACH(n, authcookie_list.head)
		{
			ac = n->data;

			if (ac->myuser == myuser)
				return ac;
		}
	}
	else			/* must have both */
	{
		MOWGLI_ITER_FOREACH(n, authcookie_list.head)
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
void
authcookie_destroy(struct authcookie * ac)
{
	return_if_fail(ac != NULL);

	mowgli_node_delete(&ac->node, &authcookie_list);
	sfree(ac->ticket);
	mowgli_heap_free(authcookie_heap, ac);
}

/*
 * authcookie_destroy_all()
 *
 * Inputs:
 *       a struct myuser pointer
 *
 * Outputs:
 *       none
 *
 * Side Effects:
 *       all authcookies for the user are destroyed
 */
void
authcookie_destroy_all(struct myuser *mu)
{
	mowgli_node_t *n, *tn;
	struct authcookie *ac;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, authcookie_list.head)
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
void
authcookie_expire(void *arg)
{
	struct authcookie *ac;
	mowgli_node_t *n, *tn;

	(void)arg;
        MOWGLI_ITER_FOREACH_SAFE(n, tn, authcookie_list.head)
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
 *       true if the authcookie is valid,
 *       otherwise false
 *
 * Side Effects:
 *       expired authcookies are destroyed here
 */
bool ATHEME_FATTR_WUR
authcookie_validate(const char *ticket, struct myuser *myuser)
{
	struct authcookie *ac = authcookie_find(ticket, myuser);

	if (ac == NULL)
		return false;

	if (ac->expire <= CURRTIME)
	{
		authcookie_destroy(ac);
		return false;
	}

	return true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
