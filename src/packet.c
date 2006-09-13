/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * IRC packet handling.
 *
 * $Id: packet.c 6371 2006-09-13 14:51:44Z jilles $
 *
 * TODO: Take all the sendq stuff in node.c and put it here.
 * sendq_flush becomes irc_whandler, etc.
 */

#include "atheme.h"
#include "uplink.h"

/* bursting timer */
#if HAVE_GETTIMEOFDAY
struct timeval burstime;
#endif

static void irc_recvq_handler(connection_t *cptr)
{
	boolean_t wasnonl;
	char parsebuf[BUFSIZE + 1];
	int count;

	wasnonl = cptr->flags & CF_NONEWLINE ? TRUE : FALSE;
	count = recvq_getline(cptr, parsebuf, sizeof parsebuf - 1);
	if (count <= 0)
		return;
	cnt.bin += count;
	/* ignore the excessive part of a too long line */
	if (wasnonl)
		return;
	me.uplinkpong = CURRTIME;
	if (parsebuf[count - 1] == '\n')
		count--;
	if (count > 0 && parsebuf[count - 1] == '\r')
		count--;
	parsebuf[count] = '\0';
	parse(parsebuf);
}

static void ping_uplink(void *arg)
{
	uint32_t diff;

	if (me.connected)
	{
		ping_sts();

		diff = CURRTIME - me.uplinkpong;

		if (diff >= 600)
		{
			slog(LG_INFO, "ping_uplink(): uplink appears to be dead, disconnecting");
			sts("ERROR :Closing Link: 127.0.0.1 (Ping timeout: %d seconds)", diff);
			sendq_flush(curr_uplink->conn);
			if (me.connected)
			{
				errno = 0;
				hook_call_event("connection_dead", curr_uplink->conn);
			}
		}
	}

	if (!me.connected)
		event_delete(ping_uplink, NULL);
}

static void irc_handle_connect(void *vptr)
{
	connection_t *cptr = vptr;

	/* add our server */

	if (cptr == curr_uplink->conn)
	{
		cptr->flags = CF_UPLINK;
		cptr->recvq_handler = irc_recvq_handler;
		me.connected = TRUE;
		/* no SERVER message received */
		me.recvsvr = FALSE;

		slog(LG_INFO, "irc_handle_connect(): connection to uplink established");

		server_login();

#ifdef HAVE_GETTIMEOFDAY
		/* start our burst timer */
		s_time(&burstime);
#endif

		/* done bursting by this time... */
		ping_sts();

		/* ping our uplink every 5 minutes */
		event_delete(ping_uplink, NULL);
		event_add("ping_uplink", ping_uplink, NULL, 300);
		me.uplinkpong = time(NULL);
	}
}

void init_ircpacket(void)
{
	hook_add_event("connected");
	hook_add_hook("connected", irc_handle_connect);

	hook_add_event("connection_dead");
	hook_add_hook("connection_dead", connection_dead);
}
