/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Services binary tree manipulation. (add_service, del_service, et al.)
 *
 * $Id: servtree.c 2963 2005-10-17 01:18:59Z jilles $
 */

#include "atheme.h"

list_t services[HASHSIZE];
static BlockHeap *service_heap;

void servtree_init(void)
{
	service_heap = BlockHeapCreate(sizeof(service_t), 12);

	if (!service_heap)
	{
		slog(LG_INFO, "servtree_init(): Block allocator failed.");
		exit(EXIT_FAILURE);
	}
}

service_t *add_service(char *name, char *user, char *host, char *real, void (*handler) (char *origin, uint8_t parc, char *parv[]))
{
	service_t *sptr;
	
	if ( name == NULL )
	{
		slog(LG_INFO, "add_service(): Bad error! We were given a NULL pointer for service name!");
		return NULL;
	}

	if ((sptr = find_service(name)))
	{
		slog(LG_DEBUG, "add_service(): Service `%s' already exists.", name);
		return NULL;
	}

	sptr = BlockHeapAlloc(service_heap);

	sptr->name = sstrdup(name);
	sptr->user = sstrdup(user);
	sptr->host = sstrdup(host);
	sptr->real = sstrdup(real);

	/* Display name, either <Service> or <Service>@<Server> */
	sptr->disp = service_name(name);

	sptr->hash = SHASH((unsigned char *)sptr->name);
	sptr->node = node_create();
	sptr->handler = handler;

	if (me.connected)
		sptr->me = introduce_nick(name, user, host, real, "io");

	node_add(sptr, sptr->node, &services[sptr->hash]);

	return sptr;
}

void del_service(service_t * sptr)
{
	node_del(sptr->node, &services[sptr->hash]);

	quit_sts(sptr->me, "Service unloaded.");
	user_delete(sptr->name);
	sptr->me = NULL;
	sptr->handler = NULL;
	free(sptr->disp);	/* service_name() does a malloc() */
	free(sptr->name);
	free(sptr->user);
	free(sptr->host);
	free(sptr->real);

	BlockHeapFree(service_heap, sptr);
}

static service_t *find_named_service(char *name)
{
	node_t *n;
	service_t *sptr;

	LIST_FOREACH(n, services[SHASH((unsigned char *)name)].head)
	{
		sptr = n->data;

		if (!strcasecmp(name, sptr->name))
			return sptr;
	}
	return NULL;
}

service_t *find_service(char *name)
{
	service_t *sptr;
	node_t *n;
	user_t *u;
	char *p;
	char name2[NICKLEN];

	if (name[0] == '#')
		return chansvs.fantasy ? chansvs.me : NULL;

	if (strchr(name, '@'))
	{
		strlcpy(name2, name, sizeof name2);
		p = strchr(name2, '@');
		if (p != NULL)
			*p = '\0';
		sptr = find_named_service(name2);
		if (sptr != NULL)
			return sptr;
		/* XXX not really nice with a 64k hashtable, so don't do
		 * it for now -- jilles */
#if 0
		for (i = 0; i < HASHSIZE; i++)
			LIST_FOREACH(n, services[i].head)
			{
				sptr = n->data;

				if (sptr->me && !strcasecmp(name2, sptr->user))
					return sptr;
			}
#endif
	}
	else
	{
		sptr = find_named_service(name);
		if (sptr != NULL)
			return sptr;

		if (ircd->uses_uid)
		{
			/* yuck yuck -- but quite efficient -- jilles */
			u = user_find(name);
			if (u != NULL && u->server == me.me)
				return find_named_service(u->nick);
		}
	}

	return NULL;
}

char *service_name(char *name)
{
	char *buf = smalloc(BUFSIZE);

	snprintf(buf, BUFSIZE, "%s%s%s", name, (config_options.secure) ? "@" : "", (config_options.secure) ? me.name : "");

	return buf;
}
