/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * packet.c: IRC packet handling.
 */

#include <atheme.h>
#include "internal.h"

/* bursting timer */
#if HAVE_GETTIMEOFDAY
struct timeval burstime;
#endif

static mowgli_eventloop_timer_t *ping_uplink_timer = NULL;

static void
irc_recvq_handler(struct connection *cptr)
{
	bool wasnonl;
	char parsebuf[BUFSIZE + 1];
	int count;

	wasnonl = CF_IS_NONEWLINE(cptr) ? true : false;
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

static void
ping_uplink(void *arg)
{
	unsigned int diff;

	if (me.connected)
	{
		ping_sts();

		diff = CURRTIME - me.uplinkpong;

		if (diff >= (10 * SECONDS_PER_MINUTE))
		{
			slog(LG_INFO, "ping_uplink(): uplink appears to be dead, disconnecting");
			sts("ERROR :Closing Link: 127.0.0.1 (Ping timeout: %u seconds)", diff);
			sendq_flush(curr_uplink->conn);
			if (me.connected)
			{
				errno = 0;
				connection_close(curr_uplink->conn);
			}
		}
	}

	if (!me.connected)
		ping_uplink_timer = NULL;
}

void
irc_handle_connect(struct connection *cptr)
{
	/* add our server */
	{
		cptr->flags = CF_UPLINK;
		cptr->recvq_handler = irc_recvq_handler;
		connection_setselect_read(cptr, recvq_put);
		slog(LG_INFO, "irc_handle_connect(): connection to uplink established");
		me.connected = true;
		/* no SERVER message received */
		me.recvsvr = false;


		server_login();

#ifdef HAVE_GETTIMEOFDAY
		/* start our burst timer */
		s_time(&burstime);
#endif

		/* done bursting by this time... */
		ping_sts();

		/* ping our uplink every 5 minutes */
		if (ping_uplink_timer != NULL)
			mowgli_timer_destroy(base_eventloop, ping_uplink_timer);

		ping_uplink_timer = mowgli_timer_add(base_eventloop, "ping_uplink", ping_uplink, NULL, 300);

		me.uplinkpong = time(NULL);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
