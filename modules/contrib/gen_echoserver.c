/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * An echo server. (proof of concept for integrated XMLRPC HTTPD)
 *
 */

#include "atheme.h"
#include "datastream.h"

DECLARE_MODULE_V1
(
	"contrib/gen_echoserver", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"William Pitcock <nenolod -at- nenolod.net>"
);

connection_t *listener;

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
        char *ptr, buf2[BUFSIZE * 2];
        static char tmp[BUFSIZE * 2 + 1];

        while ((ptr = strchr(buf, '\n')))
        {
                snprintf(buf2, (BUFSIZE * 2), "%s%s", tmp, buf);
                *tmp = '\0';

		slog(LG_DEBUG, "-{incoming}-> %s", buf2);
		sendq_add(cptr, buf2, strlen(buf2));

                buf = ptr + 1;
        }

        if (*buf)
        {
                strlcpy(tmp, buf, BUFSIZE * 2);
                tmp[BUFSIZE * 2] = '\0';
        }
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
	connection_t *newptr; 
	newptr = connection_accept_tcp(cptr, my_rhandler, NULL);
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
