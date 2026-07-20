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
int
send_line(const char *line)
{
	char buf[BUFSIZE+1];
	size_t off = 0;
	int len;

	if (!me.connected)
		return 0;

	return_val_if_fail(curr_uplink != NULL, 0);
	return_val_if_fail(curr_uplink->conn != NULL, 0);
	return_val_if_fail(line != NULL, 0);

	if (*line == '@')
	{
		const char *sp = strchr(line, ' ');
		if (sp == NULL)
			return 0;

		// right now BUFSIZE == 1024, so we only support up to 512 bytes of tags including framing
		// this should be adjusted to 8191 once the rest of atheme grows support for the longer sizes
		// if tag data is longer, skip the tags rather than truncating them to avoid corrupted tag data
		// also skip tags if the IRCd lacks support for receiving message tags
		if (ircd->flags & IRCD_MESSAGE_TAGS && sp - line < 512)
		{
			mowgli_strlcpy(buf, line, sp - line + 2);
			off = sp - line + 1;
		}

		line = sp + 1;
	}

	return_val_if_fail(sizeof(buf) - off >= 513, 0);
	mowgli_strlcpy(buf + off, line, 511);

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
