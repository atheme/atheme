/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Uplink management stuff.
 *
 * $Id: uplink.c 6931 2006-10-24 16:53:07Z jilles $
 */

#include "atheme.h"
#include "uplink.h"

list_t uplinks;
uplink_t *curr_uplink;

static BlockHeap *uplink_heap;

static void uplink_close(connection_t *cptr);

void init_uplinks(void)
{
	uplink_heap = BlockHeapCreate(sizeof(uplink_t), 4);
	if (!uplink_heap)
	{
		slog(LG_INFO, "init_uplinks(): block allocator failed.");
		exit(EXIT_FAILURE);
	}
}

uplink_t *uplink_add(char *name, char *host, char *password, char *vhost, int port)
{
	uplink_t *u;
	node_t *n;

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
		u = BlockHeapAlloc(uplink_heap);
		n = node_create();
		u->node = n;
		node_add(u, n, &uplinks);
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
	node_t *n = node_find(u, &uplinks);

	free(u->name);
	free(u->host);
	free(u->pass);
	free(u->vhost);

	node_del(n, &uplinks);
	node_free(n);

	BlockHeapFree(uplink_heap, u);
	cnt.uplink--;
}

uplink_t *uplink_find(char *name)
{
	node_t *n;

	LIST_FOREACH(n, uplinks.head)
	{
		uplink_t *u = n->data;

		if (!strcasecmp(u->name, name))
			return u;
	}

	return NULL;
}

void uplink_connect(void)
{
	uplink_t *u;

	if (curr_uplink == NULL)
	{
		if (uplinks.head == NULL)
		{
			slog(LG_ERROR, "uplink_connect(): no uplinks configured, exiting. Make sure to have at least one uplink{} block in your configuration file.");
			exit(EXIT_FAILURE);
		}
		curr_uplink = uplinks.head->data;
		slog(LG_INFO, "uplink_connect(): connecting to first entry %s[%s].", curr_uplink->name, curr_uplink->host);
	}
	else if (curr_uplink->node->next)
	{
		u = curr_uplink->node->next->data;

		curr_uplink = u;
		slog(LG_INFO, "uplink_connect(): trying alternate uplink %s[%s]", curr_uplink->name, curr_uplink->host);
	}
	else
	{
		curr_uplink = uplinks.head->data;
		slog(LG_INFO, "uplink_connect(): trying again first entry %s[%s]", curr_uplink->name, curr_uplink->host);
	}

	u = curr_uplink;
	
	curr_uplink->conn = connection_open_tcp(u->host, u->vhost, u->port, recvq_put, sendq_flush);
	curr_uplink->conn->close_handler = uplink_close;
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
	event_add_once("reconn", reconn, NULL, me.recontime);

	me.connected = FALSE;

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
}
