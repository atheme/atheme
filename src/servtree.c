/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * servtree.c: Services binary tree manipulation. (add_service, 
 *    del_service, et al.)
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

mowgli_patricia_t *services;
static BlockHeap *service_heap;

static void dummy_handler(sourceinfo_t *si, int parc, char **parv)
{
}

void servtree_init(void)
{
	service_heap = BlockHeapCreate(sizeof(service_t), 12);
	services = mowgli_patricia_create(strcasecanon);

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

service_t *add_service(char *name, char *user, char *host, char *real, void (*handler) (sourceinfo_t *si, int parc, char *parv[]), list_t *cmdtree)
{
	service_t *sptr;
	user_t *u;

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

	sptr->handler = handler;
	sptr->notice_handler = dummy_handler;

	sptr->cmdtree = cmdtree;
	sptr->chanmsg = FALSE;

	if (me.connected)
	{
		u = user_find_named(name);
		if (u != NULL)
		{
			kill_user(NULL, u, "Nick taken by service");
		}
	}

	sptr->me = user_add(name, user, host, NULL, NULL, ircd->uses_uid ? uid_get() : NULL, real, me.me, CURRTIME);
	sptr->me->flags |= UF_IRCOP;

	if (me.connected)
	{
		if (!ircd->uses_uid)
			kill_id_sts(NULL, sptr->name, "Attempt to use service nick");
		introduce_nick(sptr->me);
		/* if the snoop channel already exists, join it now */
		if (config_options.chan != NULL && channel_find(config_options.chan) != NULL)
			join(config_options.chan, name);
	}

	mowgli_patricia_add(services, sptr->name, sptr);

	return sptr;
}

void del_service(service_t * sptr)
{
	mowgli_patricia_delete(services, sptr->name);

	quit_sts(sptr->me, "Service unloaded.");
	user_delete(sptr->me);
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
	return mowgli_patricia_retrieve(services, name);
}

char *service_name(char *name)
{
	char *str;

	if (config_options.secure)
	{
		str = smalloc(strlen(name) + 1 + strlen(me.name) + 1);
		sprintf(str, "%s@%s", name, me.name);
	}
	else
		str = sstrdup(name);

	return str;
}

void service_set_chanmsg(service_t *service, boolean_t chanmsg)
{
	return_if_fail(service != NULL);

	service->chanmsg = chanmsg;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
