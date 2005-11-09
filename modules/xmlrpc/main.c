/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * XMLRPC server code.
 *
 * $Id: main.c 3755 2005-11-09 23:48:04Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"xmlrpc/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 3755 2005-11-09 23:48:04Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

connection_t *listener;

static connection_t *request;

list_t conf_xmlrpc_table;

struct xmlrpc_configuration
{
	char *host;
	int port;
} xmlrpc_config;

/***************************************************************************/

static int conf_xmlrpc_host(CONFIGENTRY *ce)
{
	if (!ce->ce_vardata)
		return -1;

	xmlrpc_config.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int conf_xmlrpc_port(CONFIGENTRY *ce)
{
	if (!ce->ce_vardata)
		return -1;

	xmlrpc_config.port = ce->ce_vardatanum;

	return 0;
}

static int conf_xmlrpc(CONFIGENTRY *ce)
{
	subblock_handler(ce, &conf_xmlrpc_table);
}

/***************************************************************************/

static int testMethod(void *conn, int parc, char *parv[])
{
	char buf[XMLRPC_BUFSIZE];
	char buf2[XMLRPC_BUFSIZE];

	(void)conn;
	if (parc > 0)
		snprintf(buf2, XMLRPC_BUFSIZE, "return: %s", parv[0]);
	else
		snprintf(buf2, XMLRPC_BUFSIZE, "no arguments provided");

	xmlrpc_string(buf, buf2);
	xmlrpc_send(1, buf);
	return 0;
}

/***************************************************************************/

static char *dump_buffer(char *buf, int length)
{
	connection_write_raw(request, buf);
	sendq_flush(request);
	connection_close(request);
}

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

	xmlrpc_process(buf, cptr);

	*buf = '\0';
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
	slog(LG_DEBUG, "do_listen(): accepted xmlrpc from %s fd %d", newptr->hbuf, newptr->fd);
}

static void xmlrpc_config_ready(void *vptr)
{
	listener = connection_open_listener_tcp(xmlrpc_config.host,
		xmlrpc_config.port, do_listen);
}

void _modinit(module_t *m)
{
	if (!cold_start)
		listener = connection_open_listener_tcp(xmlrpc_config.host,
			xmlrpc_config.port, do_listen);
	else
	{
		hook_add_event("config_ready");
		hook_add_hook("config_ready", xmlrpc_config_ready);
	}

	add_top_conf("XMLRPC", conf_xmlrpc);
	add_conf_item("HOST", &conf_xmlrpc_table, conf_xmlrpc_host);
	add_conf_item("PORT", &conf_xmlrpc_table, conf_xmlrpc_port);

	xmlrpc_set_buffer(dump_buffer);
	xmlrpc_set_options(XMLRPC_HTTP_HEADER, XMLRPC_ON);
	xmlrpc_register_method("test.method", testMethod);
}

void _moddeinit(void)
{
	connection_close(listener);

	xmlrpc_unregister_method("test.method");
	del_conf_item("HOST", &conf_xmlrpc_table);
	del_conf_item("PORT", &conf_xmlrpc_table);
	del_top_conf("XMLRPC");
}
