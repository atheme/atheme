/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains socket routines.
 * Based off of W. Campbell's code.
 *
 * $Id: send.c 5065 2006-04-14 03:55:44Z w00t $
 */

#include "atheme.h"

/* send a line to the server, append the \r\n */
int8_t sts(char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	int16_t len;

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
	channel_t *c;
	node_t *n, *tn;

	if (me.connected)
		return;

	slog(LG_DEBUG, "reconn(): ----------------------- clearing -----------------------");

	/* we have to kill everything.
	 * we do not clear users here because when you delete a server,
	 * it deletes its users
	 */
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH_SAFE(n, tn, servlist[i].head)
		{
			s = (server_t *)n->data;
			if (s != me.me)
				server_delete(s->name);
		}
	}
	/* remove all the channels left */
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH_SAFE(n, tn, chanlist[i].head)
		{
			c = (channel_t *)n->data;
			channel_delete(c->name);
		}
	}
	/* this leaves me.me and all users on it (i.e. services) */

	slog(LG_DEBUG, "reconn(): ------------------------- done -------------------------");

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
