/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * XMLRPC server code.
 *
 * $Id: main.c 2425 2005-09-28 05:00:36Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"xmlrpc/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 2425 2005-09-28 05:00:36Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

connection_t *listener;

static connection_t *request;

static int my_read(connection_t *cptr, char *buf)
{
        int n;

        if ((n = read(cptr->fd, buf, BUFSIZE)) > 0)
        {
                buf[n] = '\0';
                cnt.bin += n;
        }

        return n;
}

static void do_packet(connection_t *cptr, char *buf)
{
	/* so we can write our response back later */
	request = cptr;

	xmlrpc_process(buf);
}

static void my_rhandler(connection_t * cptr)
{
        char buf[BUFSIZE * 2];

        if (!my_read(cptr, buf))
		connection_close(cptr);

        do_packet(cptr, buf);
}

static void do_listen(connection_t *cptr)
{
	connection_t *newptr = connection_accept_tcp(cptr,
		my_rhandler, sendq_flush);
	slog(LG_DEBUG, "do_listen(): accepted %d", cptr->fd);
}

void _modinit(module_t *m)
{
	listener = connection_open_listener_tcp("127.0.0.1", 7100, do_listen);
}

void _moddeinit(void)
{
	connection_close(listener);
}
