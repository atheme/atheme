/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Services binary tree manipulation. (add_service, del_service, et al.)
 *
 * $Id: servtree.c 5085 2006-04-14 12:33:34Z jilles $
 */

#include "atheme.h"

list_t services[HASHSIZE];
static BlockHeap *service_heap;

service_t *fcmd_agent = NULL;

static void dummy_handler(char *origin, uint8_t parc, char **parv);

static void dummy_handler(char *origin, uint8_t parc, char **parv)
{
}

void servtree_init(void)
{
	service_heap = BlockHeapCreate(sizeof(service_t), 12);

	if (!service_heap)
	{
		slog(LG_INFO, "servtree_init(): Block allocator failed.");
		exit(EXIT_FAILURE);
	}
}

static void me_me_init(void)
{
	if (me.numeric)
		init_uid();
	me.me = server_add(me.name, 0, NULL, me.numeric ? me.numeric : NULL, me.desc);
}

service_t *add_service(char *name, char *user, char *host, char *real, void (*handler) (char *origin, uint8_t parc, char *parv[]))
{
	service_t *sptr;

	if (me.me == NULL)
		me_me_init();
	
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

	if (me.numeric && *me.numeric)
		sptr->uid = sstrdup(uid_get());

	sptr->hash = SHASH((unsigned char *)sptr->name);
	sptr->node = node_create();
	sptr->handler = handler;
	sptr->notice_handler = dummy_handler;

	sptr->me = user_add(name, user, host, NULL, NULL, ircd->uses_uid ? sptr->uid : NULL, real, me.me, CURRTIME);
	sptr->me->flags |= UF_IRCOP;

	if (me.connected)
	{
		introduce_nick(name, user, host, real, sptr->uid);
		/* if the snoop channel already exists, join it now */
		if (config_options.chan != NULL && channel_find(config_options.chan) != NULL)
			join(config_options.chan, name);
	}

	node_add(sptr, sptr->node, &services[sptr->hash]);

	return sptr;
}

void del_service(service_t * sptr)
{
	node_del(sptr->node, &services[sptr->hash]);

	quit_sts(sptr->me, "Service unloaded.");
	user_delete(sptr->me);
	sptr->me = NULL;
	sptr->handler = NULL;
	free(sptr->disp);	/* service_name() does a malloc() */
	free(sptr->name);
	free(sptr->user);
	free(sptr->host);
	free(sptr->real);
	free(sptr->uid);

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
	user_t *u;
	char *p;
	char name2[NICKLEN];

	if (name[0] == '#')
		return fcmd_agent;

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
