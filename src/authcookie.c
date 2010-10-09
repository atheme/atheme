/*
 * atheme-services: A collection of minimalist IRC services   
 * authcookie.c: Remote authentication ticket management
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
#include "authcookie.h"

mowgli_list_t authcookie_list;
mowgli_heap_t *authcookie_heap;

void authcookie_init(void)
{
	authcookie_heap = mowgli_heap_create(sizeof(authcookie_t), 1024, BH_NOW);

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
	authcookie_t *au = mowgli_heap_alloc(authcookie_heap);

	au->ticket = gen_pw(20);
	au->myuser = mu;
	au->expire = CURRTIME + 3600;

	mowgli_node_add(au, &au->node, &authcookie_list);

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
	mowgli_node_t *n;
	authcookie_t *ac;

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
void authcookie_destroy(authcookie_t * ac)
{
	return_if_fail(ac != NULL);

	mowgli_node_delete(&ac->node, &authcookie_list);
	free(ac->ticket);
	mowgli_heap_free(authcookie_heap, ac);
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
	mowgli_node_t *n, *tn;
	authcookie_t *ac;

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
void authcookie_expire(void *arg)
{
	authcookie_t *ac;
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
bool authcookie_validate(char *ticket, myuser_t *myuser)
{
	authcookie_t *ac = authcookie_find(ticket, myuser);

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
