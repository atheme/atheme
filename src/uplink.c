/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Uplink management stuff.
 *
 * $Id: uplink.c 3307 2005-10-31 00:24:06Z jilles $
 */

#include "atheme.h"

uplink_t *curr_uplink;

void uplink_connect(void)
{
	uplink_t *u;

	if (curr_uplink == NULL)
	{
		curr_uplink = uplinks.head->data;
		slog(LG_DEBUG, "uplink_connect(): connecting to first entry `%s'.", curr_uplink->name);
	}
	else if (curr_uplink->node->next)
	{
		u = curr_uplink->node->next->data;

		curr_uplink = u;
		slog(LG_DEBUG, "uplink_connect(): trying alternate uplink `%s'", curr_uplink->name);
	}
	else
	{
		curr_uplink = uplinks.head->data;
		slog(LG_DEBUG, "uplink_connect(): trying again first entry `%s'", curr_uplink->name);
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
                event_add_once("reconn", reconn, NULL, me.recontime);

        connection_close(cptr);

	me.connected = FALSE;
}

