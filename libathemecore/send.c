/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2011 Atheme Project (http://atheme.org/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * send.c: Socket I/O.
 */

#include <atheme.h>
#include "internal.h"

/* send a line to the server, append the \r\n */
int ATHEME_FATTR_PRINTF(1, 2)
sts(const char *fmt, ...)
{
	va_list ap;
	char buf[513];
	int len;

	if (!me.connected)
		return 0;

	return_val_if_fail(curr_uplink != NULL, 0);
	return_val_if_fail(curr_uplink->conn != NULL, 0);
	return_val_if_fail(fmt != NULL, 0);

	va_start(ap, fmt);
	vsnprintf(buf, 511, fmt, ap); /* leave two bytes for \r\n */
	va_end(ap);

	len = strlen(buf);
	buf[len++] = '\r';
	buf[len++] = '\n';
	buf[len] = '\0';

	cnt.bout += len;

	sendq_add(curr_uplink->conn, buf, len);

	slog(LG_RAWDATA, "<- %.*s", len, buf);

	return 0;
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
void
io_loop(void)
{
	while (!(runflags & (RF_SHUTDOWN | RF_RESTART)))
	{
		CURRTIME = mowgli_eventloop_get_time(base_eventloop);
		mowgli_eventloop_run_once(base_eventloop);
		check_signals();
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
