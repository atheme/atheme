/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Uplink management stuff.
 *
 * $Id: uplink.c 5319 2006-05-27 23:01:49Z jilles $
 */

#include "atheme.h"

uplink_t *curr_uplink;

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
	
	curr_uplink->conn = connection_open_tcp(u->host, u->vhost, u->port, irc_rhandler, sendq_flush);
}

/*
 * connection_dead()
 * 
 * inputs:
 *       void pointer pointing to connection nodelet,
 *       triggered by event connection_dead.
 *
 * outputs:
 *       none
 *
 * side effects:
 *       the connection is closed and shut down.
 */
void connection_dead(void *vptr)
{
        connection_t *cptr = vptr;

        if (cptr == curr_uplink->conn)
	{
                event_add_once("reconn", reconn, NULL, me.recontime);

		me.connected = FALSE;

		if (curr_uplink->flags & UPF_ILLEGAL)
		{
			slog(LG_INFO, "connection_dead(): %s was removed from configuration, deleting", curr_uplink->name);
			uplink_delete(curr_uplink);
			if (uplinks.head == NULL)
			{
				slog(LG_ERROR, "connection_dead(): last uplink deleted, exiting.");
				exit(EXIT_FAILURE);
			}
			curr_uplink = uplinks.head->data;
		}
		curr_uplink->conn = NULL;
	}

        connection_close(cptr);
}

