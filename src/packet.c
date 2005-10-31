/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * IRC packet handling.
 *
 * $Id: packet.c 3307 2005-10-31 00:24:06Z jilles $
 *
 * TODO: Take all the sendq stuff in node.c and put it here.
 * sendq_flush becomes irc_whandler, etc.
 */

#include "atheme.h"

static int irc_read(connection_t * cptr, char *buf)
{
	int n;

#ifdef _WIN32
	if ((n = recv(cptr->fd, buf, BUFSIZE, 0)) > 0)
#else
	if ((n = read(cptr->fd, buf, BUFSIZE)) > 0)
#endif
	{
		buf[n] = '\0';
		cnt.bin += n;
	}

	return n;
}

static void irc_packet(char *buf)
{
	char *iptr;
	static char parsebuf[BUFSIZE * 2 + 1];
	static char *pptr = parsebuf;

	for (iptr = buf; *iptr != '\0'; )
	{
		if (*iptr == '\n')
		{
			*pptr = '\0';
			if (*(iptr-1) == '\r')
				*(pptr-1) = '\0';
			iptr++;
			me.uplinkpong = CURRTIME;
			parse(parsebuf);
			pptr = parsebuf;
		}
		else
			*pptr++ = *iptr++;
	}
}

void irc_rhandler(connection_t * cptr)
{
	char buf[BUFSIZE * 2];

	if (!irc_read(cptr, buf))
	{
		slog(LG_INFO, "io_loop(): lost connection to uplink.");
		hook_call_event("connection_dead", cptr);
		me.connected = FALSE;
	}

	irc_packet(buf);
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
	uint8_t ret;
	connection_t *cptr = vptr;

	/* add our server */

	if (cptr == curr_uplink->conn)
	{
		cptr->flags = CF_UPLINK;
		me.connected = TRUE;

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
