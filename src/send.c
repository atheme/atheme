/*
 * atheme-services: A collection of minimalist IRC services
 * send.c: Socket I/O.
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

/* send a line to the server, append the \r\n */
int sts(const char *fmt, ...)
{
	va_list ap;
	char buf[513];
	int len;

	return_val_if_fail(fmt != NULL, 1);

	va_start(ap, fmt);
	vsnprintf(buf, 511, fmt, ap); /* leave two bytes for \r\n */
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
	int t;

	while (!(runflags & (RF_SHUTDOWN | RF_RESTART)))
	{
		/* update the current time */
		CURRTIME = time(NULL);

		/* check for events */
		delay = event_next_time();

		if (delay <= CURRTIME)
		{
			event_run();
			CURRTIME = time(NULL);
			delay = event_next_time();
		}

		if (delay <= CURRTIME)
			t = 250;
		else
			t = (delay - CURRTIME) * 1000;
		connection_select(t);

		/* actually handle signals when it's safe to do so -- jilles */
		check_signals();
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
