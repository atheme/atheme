/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Services binary tree manipulation. (add_service, del_service, et al.)
 *
 * $Id: servtree.c 1518 2005-08-05 01:35:22Z alambert $
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

service_t *add_service(char *name, char *user, char *host, char *real,
	void (*handler)(char *origin, uint8_t parc, char *parv[]))
{
	service_t *sptr;

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

void del_service(service_t *sptr)
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

service_t *find_service(char *name)
{
	service_t *sptr;
	node_t *n;
	char *svs;

	if (name[0] == '#' && chansvs.fantasy)
		return chansvs.me;

	if (strchr(name, '@'))
		svs = strtok(name, "@");
	else
		svs = name;

	LIST_FOREACH(n, services[SHASH((unsigned char *)svs)].head)
	{
		sptr = n->data;

		if (!strcasecmp(svs, sptr->name))
			return sptr;
	}

	return NULL;
}

char *service_name(char *name)
{
	char *buf = smalloc(BUFSIZE);

	snprintf(buf, BUFSIZE, "%s%s%s",
		 name, (config_options.secure) ? "@" : "",
		 (config_options.secure) ? me.name : "");

	return buf;
}
