/*
 * atheme-services: A collection of minimalist IRC services
 * uplink.c: Uplink management.
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
#include "datastream.h"
#include "uplink.h"

mowgli_list_t uplinks;
uplink_t *curr_uplink;

mowgli_heap_t *uplink_heap;

static void uplink_close(connection_t *cptr);

void init_uplinks(void)
{
	uplink_heap = mowgli_heap_create(sizeof(uplink_t), 4, BH_NOW);
	if (!uplink_heap)
	{
		slog(LG_INFO, "init_uplinks(): block allocator failed.");
		exit(EXIT_FAILURE);
	}
}

uplink_t *uplink_add(const char *name, const char *host, const char *password, const char *vhost, int port)
{
	uplink_t *u;
	mowgli_node_t *n;

	slog(LG_DEBUG, "uplink_add(): %s -> %s:%d", me.name, name, port);

	if ((u = uplink_find(name)))
	{
		if (u->flags & UPF_ILLEGAL)
		{
			u->flags &= ~UPF_ILLEGAL;
			free(u->name);
			free(u->host);
			free(u->pass);
			free(u->vhost);
		}
		else
		{
			slog(LG_INFO, "Duplicate uplink %s.", name);
			return NULL;
		}
	}
	else
	{
		u = mowgli_heap_alloc(uplink_heap);
		n = mowgli_node_create();
		u->node = n;
		mowgli_node_add(u, n, &uplinks);
		cnt.uplink++;
	}

	u->name = sstrdup(name);
	u->host = sstrdup(host);
	u->pass = sstrdup(password);
	if (vhost)
		u->vhost = sstrdup(vhost);
	else
		u->vhost = sstrdup("0.0.0.0");
	u->port = port;

	return u;
}

void uplink_delete(uplink_t * u)
{
	mowgli_node_t *n = mowgli_node_find(u, &uplinks);

	free(u->name);
	free(u->host);
	free(u->pass);
	free(u->vhost);

	mowgli_node_delete(n, &uplinks);
	mowgli_node_free(n);

	mowgli_heap_free(uplink_heap, u);
	cnt.uplink--;
}

uplink_t *uplink_find(const char *name)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, uplinks.head)
	{
		uplink_t *u = n->data;

		if (!strcasecmp(u->name, name))
			return u;
	}

	return NULL;
}

static void reconn(void *arg)
{
	if (me.connected)
		return;

	uplink_connect();
}

void uplink_connect(void)
{
	uplink_t *u;

	if (curr_uplink == NULL)
	{
		if (uplinks.head == NULL)
		{
			slog(LG_ERROR, "uplink_connect(): no uplinks configured, not connecting to IRC.  For IRC usage, make sure to have at least one uplink{} block in your configuration file.");
			return;
		}
		curr_uplink = uplinks.head->data;
		slog(LG_INFO, "uplink_connect(): connecting to first entry %s[%s]:%d.", curr_uplink->name, curr_uplink->host, curr_uplink->port);
	}
	else if (curr_uplink->node->next)
	{
		u = curr_uplink->node->next->data;

		curr_uplink = u;
		slog(LG_INFO, "uplink_connect(): trying alternate uplink %s[%s]:%d", curr_uplink->name, curr_uplink->host, curr_uplink->port);
	}
	else
	{
		curr_uplink = uplinks.head->data;
		slog(LG_INFO, "uplink_connect(): trying again first entry %s[%s]:%d", curr_uplink->name, curr_uplink->host, curr_uplink->port);
	}

	u = curr_uplink;
	
	curr_uplink->conn = connection_open_tcp(u->host, u->vhost, u->port, NULL, irc_handle_connect);
	if (curr_uplink->conn != NULL)
	{
		curr_uplink->conn->close_handler = uplink_close;
		sendq_set_limit(curr_uplink->conn, config_options.uplink_sendq_limit);
	}
	else
		event_add_once("reconn", reconn, NULL, me.recontime);
}

/*
 * uplink_close()
 * 
 * inputs:
 *       connection pointer of current uplink
 *       triggered by callback close_handler
 *
 * outputs:
 *       none
 *
 * side effects:
 *       reconnection is scheduled
 *       uplink marked dead
 *       uplink deleted if it had been removed from configuration
 */
static void uplink_close(connection_t *cptr)
{
	channel_t *c;
	mowgli_patricia_iteration_state_t state;

	event_add_once("reconn", reconn, NULL, me.recontime);

	me.connected = false;

	if (curr_uplink->flags & UPF_ILLEGAL)
	{
		slog(LG_INFO, "uplink_close(): %s was removed from configuration, deleting", curr_uplink->name);
		uplink_delete(curr_uplink);
		if (uplinks.head == NULL)
		{
			slog(LG_ERROR, "uplink_close(): last uplink deleted, exiting.");
			exit(EXIT_FAILURE);
		}
		curr_uplink = uplinks.head->data;
	}
	curr_uplink->conn = NULL;

	slog(LG_DEBUG, "uplink_close(): ----------------------- clearing -----------------------");

	/* we have to kill everything.
	 * we do not clear users here because when you delete a server,
	 * it deletes its users
	 */
	if (me.actual != NULL)
		server_delete(me.actual);
	me.actual = NULL;
	/* remove all the channels left */
	MOWGLI_PATRICIA_FOREACH(c, &state, chanlist)
	{
		channel_delete(c);
	}
	/* this leaves me.me and all users on it (i.e. services) */

	slog(LG_DEBUG, "uplink_close(): ------------------------- done -------------------------");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
