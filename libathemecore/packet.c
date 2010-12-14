/*
 * atheme-services: A collection of minimalist IRC services
 * packet.c: IRC packet handling.
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
#include "uplink.h"
#include "datastream.h"

/* bursting timer */
#if HAVE_GETTIMEOFDAY
struct timeval burstime;
#endif

static void irc_recvq_handler(connection_t *cptr)
{
	bool wasnonl;
	char parsebuf[BUFSIZE + 1];
	int count;

	wasnonl = cptr->flags & CF_NONEWLINE ? true : false;
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
	unsigned int diff;

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
				connection_close(curr_uplink->conn);
			}
		}
	}

	if (!me.connected)
		event_delete(ping_uplink, NULL);
}

void irc_handle_connect(connection_t *cptr)
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
		event_delete(ping_uplink, NULL);
		event_add("ping_uplink", ping_uplink, NULL, 300);
		me.uplinkpong = time(NULL);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
