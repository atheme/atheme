/*
 * atheme-services: A collection of minimalist IRC services
 * servers.c: Server and network state tracking.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

mowgli_dictionary_t *sidlist;
mowgli_dictionary_t *servlist;
list_t tldlist;

static BlockHeap *serv_heap;
static BlockHeap *tld_heap;

/*
 * init_servers()
 *
 * Initializes the server heap and server/sid DTree structures.
 *
 * Inputs:
 *     - nothing
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - if the heap or dtrees fail to initialize, the program
 *       will abort.
 */
void init_servers(void)
{
	serv_heap = BlockHeapCreate(sizeof(server_t), HEAP_SERVER);
	tld_heap = BlockHeapCreate(sizeof(tld_t), 4);

	if (serv_heap == NULL || tld_heap == NULL)
	{
		slog(LG_INFO, "init_servers(): block allocator failure.");
		exit(EXIT_FAILURE);
	}

	servlist = mowgli_dictionary_create(irccasecmp);
	sidlist = mowgli_dictionary_create(strcmp);
}

/*
 * server_add(const char *name, unsigned int hops, const char *uplink,
 *            const char *id, const char *desc)
 *
 * Server object factory.
 *
 * Inputs:
 *     - name of server object to create
 *     - amount of hops server has from services
 *     - name of server's uplink or NULL if it's us
 *     - SID of uplink if applicable otherwise NULL
 *     - server's description
 *
 * Outputs:
 *     - on success, a new server object
 *
 * Side Effects:
 *     - the new server object is added to the server and sid DTree.
 */
server_t *server_add(const char *name, unsigned int hops, const char *uplink, const char *id, const char *desc)
{
	server_t *s, *u = NULL;
	const char *tld;

	if (uplink)
	{
		if (id != NULL)
			slog(LG_NETWORK, "server_add(): %s (%s), uplink %s", name, id, uplink);
		else
			slog(LG_NETWORK, "server_add(): %s, uplink %s", name, uplink);
		u = server_find(uplink);
	}
	else
		slog(LG_DEBUG, "server_add(): %s, root", name);

	s = BlockHeapAlloc(serv_heap);

	if (id != NULL)
	{
		s->sid = sstrdup(id);
		mowgli_dictionary_add(sidlist, s->sid, s);
	}

	/* check to see if it's hidden */
	if (!strncmp(desc, "(H)", 3))
	{
		s->flags |= SF_HIDE;
		desc += 3;
		if (*desc == ' ')
			desc++;
	}

	s->name = sstrdup(name);
	s->desc = sstrdup(desc);
	s->hops = hops;
	s->connected_since = CURRTIME;

	mowgli_dictionary_add(servlist, s->name, s);

	if (u)
	{
		s->uplink = u;
		node_add(s, node_create(), &u->children);
	}

	/* tld list for global noticer */
	tld = strrchr(name, '.');

	if (tld != NULL)
	{
		if (!tld_find(tld))
			tld_add(tld);
	}

	cnt.server++;

	return s;
}

/*
 * server_delete(const char *name)
 *
 * Finds and recursively destroys a server object.
 *
 * Inputs:
 *     - name of server to find and destroy
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - all users and servers attached to the target are recursively deleted
 */
void server_delete(const char *name)
{
	server_t *s = server_find(name);
	server_t *child;
	user_t *u;
	node_t *n, *tn;

	if (!s)
	{
		slog(LG_DEBUG, "server_delete(): called for nonexistant server: %s", name);

		return;
	}
	if (s == me.me)
	{
		/* Deleting this would cause confusion, so let's not do it.
		 * Some ircds send SQUIT <myname> when atheme is squitted.
		 * -- jilles
		 */
		slog(LG_DEBUG, "server_delete(): tried to delete myself");
		return;
	}

	slog(me.connected ? LG_NETWORK : LG_DEBUG, "server_delete(): %s, uplink %s (%d users)",
			s->name, s->uplink != NULL ? s->uplink->name : "<none>",
			s->users);

	/* first go through it's users and kill all of them */
	LIST_FOREACH_SAFE(n, tn, s->userlist.head)
	{
		u = (user_t *)n->data;
		/* This user split, allow bursted logins for the account.
		 * XXX should we do this here?
		 * -- jilles */
		if (u->myuser != NULL)
			u->myuser->flags &= ~MU_NOBURSTLOGIN;
		user_delete(u);
	}

	LIST_FOREACH_SAFE(n, tn, s->children.head)
	{
		child = n->data;
		server_delete(child->name);
	}

	/* now remove the server */
	mowgli_dictionary_delete(servlist, s->name);

	if (s->sid)
		mowgli_dictionary_delete(sidlist, s->sid);

	if (s->uplink)
	{
		n = node_find(s, &s->uplink->children);
		node_del(n, &s->uplink->children);
		node_free(n);
	}

	/* If unconnect semantics SQUIT was confirmed, introduce the jupe
	 * now. This must be after removing the server from the dtrees.
	 * -- jilles */
	if (s->flags & SF_JUPE_PENDING)
		jupe(s->name, "Juped");

	free(s->name);
	free(s->desc);
	if (s->sid)
		free(s->sid);

	BlockHeapFree(serv_heap, s);

	cnt.server--;
}

/*
 * server_find(const char *name)
 *
 * Finds a server object.
 *
 * Inputs:
 *     - name of server to find
 *
 * Outputs:
 *     - on success, the server object
 *     - on failure, NULL.
 *
 * Side Effects:
 *     - none
 */
server_t *server_find(const char *name)
{
	server_t *s;

	if (ircd->uses_uid)
	{
		s = mowgli_dictionary_retrieve(sidlist, name);
		if (s != NULL)
			return s;
	}

	return mowgli_dictionary_retrieve(servlist, name);
}

/*
 * tld_add(const char *name)
 *
 * TLD object factory.
 *
 * Inputs:
 *     - name of TLD to cache as an object
 *
 * Outputs:
 *     - on success, a TLD object
 *     - on failure, NULL
 *
 * Side Effects:
 *     - the TLD object is registered with the TLD list.
 */
tld_t *tld_add(const char *name)
{
        tld_t *tld;
        node_t *n = node_create();

        slog(LG_DEBUG, "tld_add(): %s", name);

        tld = BlockHeapAlloc(tld_heap);

        node_add(tld, n, &tldlist);

        tld->name = sstrdup(name);

        cnt.tld++;

        return tld;
}

/*
 * tld_delete(const char *name)
 *
 * Destroys a TLD object.
 *
 * Inputs:
 *     - name of TLD object to destroy
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - the TLD object is removed and deregistered from the TLD list.
 */
void tld_delete(const char *name)
{
        tld_t *tld = tld_find(name);
        node_t *n;

        if (!tld)
        {
                slog(LG_DEBUG, "tld_delete(): called for nonexistant tld: %s", name);

                return;
        }

        slog(LG_DEBUG, "tld_delete(): %s", tld->name);

        n = node_find(tld, &tldlist);
        node_del(n, &tldlist);
        node_free(n);

        free(tld->name);
        BlockHeapFree(tld_heap, tld);

        cnt.tld--;
}

/*
 * tld_find(const char *name)
 *
 * Looks up a TLD object.
 *
 * Inputs:
 *     - name of TLD object to look up
 *
 * Outputs:
 *     - on success, the TLD object
 *     - on failure, NULL
 *
 * Side Effects:
 *     - none
 */
tld_t *tld_find(const char *name)
{
        tld_t *tld;
        node_t *n;

	if (name == NULL)
		return NULL;

        LIST_FOREACH(n, tldlist.head)
        {
                tld = (tld_t *)n->data;

                if (!strcasecmp(name, tld->name))
                        return tld;
        }

        return NULL;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
