/*
 * atheme-services: A collection of minimalist IRC services
 * servers.c: Server and network state tracking.
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

mowgli_patricia_t *sidlist;
mowgli_patricia_t *servlist;
mowgli_list_t tldlist;

mowgli_heap_t *serv_heap;
mowgli_heap_t *tld_heap;

static void server_delete_serv(server_t *s);

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
	serv_heap = mowgli_heap_create(sizeof(server_t), HEAP_SERVER, BH_NOW);
	tld_heap = mowgli_heap_create(sizeof(tld_t), 4, BH_NOW);

	if (serv_heap == NULL || tld_heap == NULL)
	{
		slog(LG_INFO, "init_servers(): block allocator failure.");
		exit(EXIT_FAILURE);
	}

	servlist = mowgli_patricia_create(irccasecanon);
	sidlist = mowgli_patricia_create(noopcanon);
}

/*
 * server_add(const char *name, unsigned int hops, const char *uplink,
 *            const char *id, const char *desc)
 *
 * Server object factory.
 *
 * Inputs:
 *     - name of server object to create or NULL if it's a masked server
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
server_t *server_add(const char *name, unsigned int hops, server_t *uplink, const char *id, const char *desc)
{
	server_t *s;
	const char *tld;

	/* Masked servers must have a SID */
	return_val_if_fail(name != NULL || id != NULL, NULL);
	/* Masked servers must be behind something else */
	return_val_if_fail(name != NULL || uplink != NULL, NULL);

	if (uplink)
	{
		if (name == NULL)
			slog(LG_NETWORK, "server_add(): %s (%s), masked", uplink->name, id);
		else if (id != NULL)
			slog(LG_NETWORK, "server_add(): %s (%s), uplink %s", name, id, uplink->name);
		else
			slog(LG_NETWORK, "server_add(): %s, uplink %s", name, uplink->name);
	}
	else
		slog(LG_DEBUG, "server_add(): %s, root", name);

	s = mowgli_heap_alloc(serv_heap);

	if (id != NULL)
	{
		s->sid = sstrdup(id);
		mowgli_patricia_add(sidlist, s->sid, s);
	}

	/* check to see if it's hidden */
	if (!strncmp(desc, "(H)", 3))
	{
		s->flags |= SF_HIDE;
		desc += 3;
		if (*desc == ' ')
			desc++;
	}

	s->name = sstrdup(name != NULL ? name : uplink->name);
	s->desc = sstrdup(desc);
	s->hops = hops;
	s->connected_since = CURRTIME;

	if (name != NULL)
		mowgli_patricia_add(servlist, s->name, s);
	else
		s->flags |= SF_MASKED;

	if (uplink)
	{
		s->uplink = uplink;
		mowgli_node_add(s, mowgli_node_create(), &uplink->children);
	}

	/* tld list for global noticer */
	tld = strrchr(s->name, '.');

	if (tld != NULL)
	{
		if (!tld_find(tld))
			tld_add(tld);
	}

	cnt.server++;

	hook_call_server_add(s);

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

	if (!s)
	{
		slog(LG_DEBUG, "server_delete(): called for nonexistant server: %s", name);

		return;
	}
	server_delete_serv(s);
}

static void server_delete_serv(server_t *s)
{
	server_t *child;
	user_t *u;
	mowgli_node_t *n, *tn;

	if (s == me.me)
	{
		/* Deleting this would cause confusion, so let's not do it.
		 * Some ircds send SQUIT <myname> when atheme is squitted.
		 * -- jilles
		 */
		slog(LG_DEBUG, "server_delete(): tried to delete myself");
		return;
	}

	if (s->sid)
		slog(me.connected ? LG_NETWORK : LG_DEBUG, "server_delete(): %s (%s), uplink %s (%d users)",
				s->name, s->sid,
				s->uplink != NULL ? s->uplink->name : "<none>",
				s->users);
	else
		slog(me.connected ? LG_NETWORK : LG_DEBUG, "server_delete(): %s, uplink %s (%d users)",
				s->name, s->uplink != NULL ? s->uplink->name : "<none>",
				s->users);

	hook_call_server_delete((&(hook_server_delete_t){ .s = s }));

	/* first go through it's users and kill all of them */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, s->userlist.head)
	{
		u = (user_t *)n->data;
		/* This user split, allow bursted logins for the account.
		 * XXX should we do this here?
		 * -- jilles */
		if (u->myuser != NULL)
			u->myuser->flags &= ~MU_NOBURSTLOGIN;
		user_delete(u, "*.net *.split");
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, s->children.head)
	{
		child = n->data;
		server_delete_serv(child);
	}

	/* now remove the server */
	if (!(s->flags & SF_MASKED))
		mowgli_patricia_delete(servlist, s->name);

	if (s->sid)
		mowgli_patricia_delete(sidlist, s->sid);

	if (s->uplink)
	{
		n = mowgli_node_find(s, &s->uplink->children);
		mowgli_node_delete(n, &s->uplink->children);
		mowgli_node_free(n);
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

	mowgli_heap_free(serv_heap, s);

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
		s = mowgli_patricia_retrieve(sidlist, name);
		if (s != NULL)
			return s;
	}

	return mowgli_patricia_retrieve(servlist, name);
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
        mowgli_node_t *n = mowgli_node_create();

        slog(LG_DEBUG, "tld_add(): %s", name);

        tld = mowgli_heap_alloc(tld_heap);

        mowgli_node_add(tld, n, &tldlist);

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
        mowgli_node_t *n;

        if (!tld)
        {
                slog(LG_DEBUG, "tld_delete(): called for nonexistant tld: %s", name);

                return;
        }

        slog(LG_DEBUG, "tld_delete(): %s", tld->name);

        n = mowgli_node_find(tld, &tldlist);
        mowgli_node_delete(n, &tldlist);
        mowgli_node_free(n);

        free(tld->name);
        mowgli_heap_free(tld_heap, tld);

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
        mowgli_node_t *n;

	if (name == NULL)
		return NULL;

        MOWGLI_ITER_FOREACH(n, tldlist.head)
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
