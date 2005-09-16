/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains socket routines.
 * Based off of W. Campbell's code.
 *
 * $Id: send.c 2253 2005-09-16 07:47:34Z nenolod $
 */

#include "atheme.h"

time_t CURRTIME;

/* send a line to the server, append the \r\n */
int8_t sts(char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	int16_t len, n;

	/* glibc sucks. */
	if (!fmt)
		return 1;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	slog(LG_DEBUG, "<- %s", buf);

	len = strlen(buf);
	buf[len++] = '\r';
	buf[len++] = '\n';
	buf[len] = '\0';

	cnt.bout += len;

	sendq_add(curr_uplink->conn, buf, len, 0);

	return 0;
}

void reconn(void *arg)
{
	uint32_t i;
	server_t *s;
	node_t *n, *tn;

	if (me.connected)
		return;

	slog(LG_DEBUG, "reconn(): ----------------------- clearing -----------------------");

	/* we have to kill everything.
	 * we only clear servers here because when you delete a server,
	 * it deletes its users, which removes them from the channels, which
	 * deletes the channels.
	 */
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH_SAFE(n, tn, servlist[i].head)
		{
			s = (server_t *)n->data;
			server_delete(s->name);
		}
	}

	slog(LG_DEBUG, "reconn(): ------------------------- done -------------------------");

	slog(LG_INFO, "reconn(): connecting to an alternate server");

	uplink_connect();
}

/*
 * io_loop()
 *
 * inputs:
 *       none
 *
 * outputs:
 *       none
 *
 * side effects:
 *       everything happens inside this loop.
 */
void io_loop(void)
{
	time_t delay;

	while (!(runflags & (RF_SHUTDOWN | RF_RESTART)))
	{
		/* update the current time */
		CURRTIME = time(NULL);

		/* check for events */
		delay = event_next_time();

		if (delay <= CURRTIME)
			event_run();

		connection_select(25000);
	}
}
