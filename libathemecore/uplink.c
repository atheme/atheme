/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * uplink.c: Uplink management.
 */

#include <atheme.h>
#include "internal.h"

static void uplink_close(struct connection *cptr);

static mowgli_heap_t *uplink_heap = NULL;

mowgli_list_t uplinks;
struct uplink *curr_uplink;

void (*parse)(char *line) = NULL;

void
init_uplinks(void)
{
	(void) memset(&uplinks, 0x00, sizeof uplinks);

	uplink_heap = sharedheap_get(sizeof(struct uplink));
	if (!uplink_heap)
	{
		slog(LG_INFO, "init_uplinks(): block allocator failed.");
		exit(EXIT_FAILURE);
	}
}

struct uplink *
uplink_add(const char *name, const char *host, const char *send_password, const char *receive_password, const char *vhost, unsigned int port)
{
	struct uplink *u;

	slog(LG_DEBUG, "uplink_add(): %s -> %s:%u", me.name, name, port);

	if ((u = uplink_find(name)))
	{
		if (u->flags & UPF_ILLEGAL)
		{
			u->flags &= ~UPF_ILLEGAL;

			sfree(u->name);
			sfree(u->host);
			sfree(u->send_pass);
			sfree(u->receive_pass);
			sfree(u->vhost);
		}
		else
		{
			slog(LG_ERROR, "Ignoring duplicate uplink %s.", name);
			return NULL;
		}
	}
	else
	{
		u = mowgli_heap_alloc(uplink_heap);
		mowgli_node_add(u, &u->node, &uplinks);
		cnt.uplink++;
	}

	u->name = sstrdup(name);
	u->host = sstrdup(host);
	u->send_pass = sstrdup(send_password);
	u->receive_pass = sstrdup(receive_password);
	u->vhost = sstrdup(vhost);
	u->port = port;

	return u;
}

void
uplink_delete(struct uplink * u)
{
	sfree(u->name);
	sfree(u->host);
	sfree(u->send_pass);
	sfree(u->receive_pass);
	sfree(u->vhost);

	mowgli_node_delete(&u->node, &uplinks);
	mowgli_heap_free(uplink_heap, u);

	cnt.uplink--;
}

struct uplink *
uplink_find(const char *name)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, uplinks.head)
	{
		struct uplink *u = n->data;

		if (!strcasecmp(u->name, name))
			return u;
	}

	return NULL;
}

static void
reconn(void *arg)
{
	if (me.connected)
		return;

	uplink_connect();
}

void
uplink_connect(void)
{
	struct uplink *u;

	if (curr_uplink == NULL)
	{
		if (uplinks.head == NULL)
		{
			slog(LG_ERROR, "uplink_connect(): no uplinks configured, not connecting to IRC.  For IRC usage, make sure to have at least one uplink{} block in your configuration file.");
			return;
		}
		curr_uplink = uplinks.head->data;
		slog(LG_INFO, "uplink_connect(): connecting to first entry %s[%s]:%u.", curr_uplink->name, curr_uplink->host, curr_uplink->port);
	}
	else if (curr_uplink->node.next)
	{
		u = curr_uplink->node.next->data;

		curr_uplink = u;
		slog(LG_INFO, "uplink_connect(): trying alternate uplink %s[%s]:%u", curr_uplink->name, curr_uplink->host, curr_uplink->port);
	}
	else
	{
		curr_uplink = uplinks.head->data;
		slog(LG_INFO, "uplink_connect(): trying again first entry %s[%s]:%u", curr_uplink->name, curr_uplink->host, curr_uplink->port);
	}

	u = curr_uplink;

	curr_uplink->conn = connection_open_tcp(u->host, u->vhost, u->port, NULL, irc_handle_connect);
	if (curr_uplink->conn != NULL)
	{
		curr_uplink->conn->close_handler = uplink_close;
		sendq_set_limit(curr_uplink->conn, config_options.uplink_sendq_limit);
	}
	else
		mowgli_timer_add_once(base_eventloop, "reconn", reconn, NULL, me.recontime);
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
static void
uplink_close(struct connection *cptr)
{
	struct channel *c;
	mowgli_patricia_iteration_state_t state;

	mowgli_timer_add_once(base_eventloop, "reconn", reconn, NULL, me.recontime);

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
