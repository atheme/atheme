/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains socket routines.
 * Based off of W. Campbell's code.
 *
 * $Id: send.c 7961 2007-03-20 17:11:45Z jilles $
 */

#include "atheme.h"
#include "uplink.h"

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

	slog(LG_RAWDATA, "<- %s", buf);

	len = strlen(buf);
	buf[len++] = '\r';
	buf[len++] = '\n';
	buf[len] = '\0';

	cnt.bout += len;

	sendq_add(curr_uplink->conn, buf, len);

	return 0;
}

void reconn(void *arg)
{
	if (me.connected)
		return;

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

		/* actually handle signals when it's safe to do so -- jilles */
		check_signals();
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
