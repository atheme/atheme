/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * IRC packet handling.
 *
 * $Id: packet.c 2185 2005-09-07 02:43:08Z nenolod $
 *
 * TODO: Take all the sendq stuff in node.c and put it here.
 * sendq_flush becomes irc_whandler, etc.
 */

#include "atheme.h"

static int irc_read(connection_t *cptr, char *buf)
{
        int n = read(cptr->fd, buf, BUFSIZE);
        buf[n] = '\0';

        cnt.bin += n;

        return n;
}

static void irc_packet(char *buf)
{
        char *ptr, buf2[BUFSIZE * 2];
        static char tmp[BUFSIZE * 2 + 1];

        while ((ptr = strchr(buf, '\n')))
        {
                *ptr = '\0';

                if (*(ptr - 1) == '\r')
                        *(ptr - 1) = '\0';

                snprintf(buf2, (BUFSIZE * 2), "%s%s", tmp, buf);
                *tmp = '\0';

                me.uplinkpong = CURRTIME;
                parse(buf2);

                buf = ptr + 1;
        }

        if (*buf)
        {
                strncpy(tmp, buf, (BUFSIZE * 2) - strlen(tmp));
                tmp[BUFSIZE * 2] = '\0';
        }
}

void irc_rhandler(connection_t *cptr)
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

        ping_sts();

        if (me.connected)
        {
                diff = CURRTIME - me.uplinkpong;

                if (diff > 600)
                {
                        slog(LG_INFO, "ping_uplink(): uplink appears to be dead");

                        me.connected = FALSE;

                        event_delete(ping_uplink, NULL);
                }
        }
}

static void irc_handle_connect(void *vptr)
{
        uint8_t ret;
	connection_t *cptr = vptr;

        /* add our server */

	if (cptr == curr_uplink->conn)
	{
		cptr->flags = CF_UPLINK;
	        me.me = server_add(me.name, 0, NULL, me.numeric ? 
			me.numeric : NULL, me.desc);
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
	        event_add("ping_uplink", ping_uplink, NULL, 300);
	        me.uplinkpong = time(NULL);
	}
}

void init_ircpacket(void)
{
	hook_add_event("connected");
	hook_add_hook("connected", irc_handle_connect);
}
