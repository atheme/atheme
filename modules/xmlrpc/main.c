/*
 * Copyright (c) 2005-2006 Jilles Tjoelker et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * New xmlrpc implementation
 *
 * $Id: main.c 6665 2006-10-05 23:45:09Z jilles $
 */

#include "atheme.h"

#define REQUEST_MAX 65536 /* maximum size of one call */
#define CONTENT_TYPE "text/xml"

DECLARE_MODULE_V1
(
	"xmlrpc/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 6665 2006-10-05 23:45:09Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void xmlrpc_command_fail(sourceinfo_t *si, faultcode_t code, const char *message);
static void xmlrpc_command_success_nodata(sourceinfo_t *si, const char *message);
static void xmlrpc_command_success_string(sourceinfo_t *si, const char *result, const char *message);

static int xmlrpcmethod_login(void *conn, int parc, char *parv[]);
static int xmlrpcmethod_logout(void *conn, int parc, char *parv[]);
static int xmlrpcmethod_command(void *conn, int parc, char *parv[]);

connection_t *listener;
connection_t *current_cptr; /* Hack: src/xmlrpc.c requires us to do this */

struct sourceinfo_vtable xmlrpc_vtable = { "xmlrpc", xmlrpc_command_fail, xmlrpc_command_success_nodata, xmlrpc_command_success_string };

/* conf stuff */
list_t conf_xmlrpc_table;
struct xmlrpc_configuration
{
	char *host;
	int port;
} xmlrpc_config;

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
	return 0;
}

struct httpddata
{
	char method[64];
	char filename[256];
	char *requestbuf;
	char *replybuf;
	int length;
	int lengthdone;
	boolean_t connection_close;
	boolean_t correct_content_type;
	boolean_t expect_100_continue;
	boolean_t sent_reply;
};

static void clear_httpddata(struct httpddata *hd)
{
	hd->method[0] = '\0';
	hd->filename[0] = '\0';
	if (hd->requestbuf != NULL)
	{
		free(hd->requestbuf);
		hd->requestbuf = NULL;
	}
	if (hd->replybuf != NULL)
	{
		free(hd->replybuf);
		hd->replybuf = NULL;
	}
	hd->length = 0;
	hd->lengthdone = 0;
	hd->correct_content_type = FALSE;
	hd->expect_100_continue = FALSE;
	hd->sent_reply = FALSE;
}

static void process_header(connection_t *cptr, char *line)
{
	struct httpddata *hd;
	char *p;

	hd = cptr->userdata;
	p = strchr(line, ':');
	if (p == NULL)
		return;
	*p = '\0';
	p++;
	while (*p == ' ')
		p++;
	if (!strcasecmp(line, "Connection"))
	{
		p = strtok(p, ", \t");
		do
		{
			if (!strcasecmp(p, "close"))
			{
				slog(LG_DEBUG, "process_header(): Connection: close requested by fd %d", cptr->fd);
				hd->connection_close = TRUE;
			}
		} while ((p = strtok(NULL, ", \t")) != NULL);
	}
	else if (!strcasecmp(line, "Content-Length"))
	{
		hd->length = atoi(p);
	}
	else if (!strcasecmp(line, "Content-Type"))
	{
		p = strtok(p, "; \t");
		hd->correct_content_type = p != NULL && !strcasecmp(p, CONTENT_TYPE);
	}
	else if (!strcasecmp(line, "Expect"))
	{
		hd->expect_100_continue = !strcasecmp(p, "100-continue");
	}
}

static void check_close(connection_t *cptr)
{
	struct httpddata *hd;

	hd = cptr->userdata;
	if (hd->connection_close)
		sendq_add_eof(cptr);
}

static void send_error(connection_t *cptr, int errorcode, const char *text, boolean_t sendentity)
{
	struct httpddata *hd;
	char buf1[300];
	char buf2[700];

	hd = cptr->userdata;
	if (errorcode < 100 || errorcode > 999)
		errorcode = 500;
	snprintf(buf2, sizeof buf2, "HTTP/1.1 %d %s\r\nAtheme XMLRPC HTTPD\r\n", errorcode, text);
	snprintf(buf1, sizeof buf1, "HTTP/1.1 %d %s\r\n"
			"Server: Atheme/%s\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: %d\r\n\r\n%s",
			errorcode, text, version, strlen(buf2),
			sendentity ? buf2 : "");
	sendq_add(cptr, buf1, strlen(buf1));
}

static void httpd_recvqhandler(connection_t *cptr)
{
	char buf[BUFSIZE * 2];
	char outbuf[BUFSIZE * 2];
	int count;
	struct httpddata *hd;
	char *p;

	hd = cptr->userdata;

	if (hd->requestbuf != NULL)
	{
		count = recvq_get(cptr, hd->requestbuf + hd->lengthdone, hd->length - hd->lengthdone);
		if (count <= 0)
			return;
		hd->lengthdone += count;
		if (hd->lengthdone != hd->length)
			return;
		hd->requestbuf[hd->length] = '\0';
		current_cptr = cptr;
		xmlrpc_process(hd->requestbuf, cptr);
		current_cptr = NULL;
		clear_httpddata(hd);
		return;
	}

	count = recvq_getline(cptr, buf, sizeof buf - 1);
	if (count <= 0)
		return;
	if (cptr->flags & CF_NONEWLINE)
	{
		slog(LG_INFO, "httpd_recvqhandler(): throwing out fd %d (%s) for excessive line length", cptr->fd, cptr->hbuf);
		send_error(cptr, 400, "Bad request", TRUE);
		sendq_add_eof(cptr);
		return;
	}
	cnt.bin += count;
	if (buf[count - 1] == '\n')
		count--;
	if (count > 0 && buf[count - 1] == '\r')
		count--;
	buf[count] = '\0';
	if (hd->method[0] == '\0')
	{
		/* make sure they're not sending more requests after
		 * declaring they're not sending any more */
		if (hd->connection_close)
			return;
		p = strtok(buf, " ");
		if (p == NULL)
			return;
		strlcpy(hd->method, p, sizeof hd->method);
		p = strtok(NULL, " ");
		if (p == NULL)
			return;
		strlcpy(hd->filename, p, sizeof hd->filename);
		p = strtok(NULL, "");
		if (p == NULL || !strcmp(p, "HTTP/1.0"))
			hd->connection_close = TRUE;
		slog(LG_DEBUG, "httpd_recvqhandler(): request %s for %s", hd->method, hd->filename);
	}
	else if (count == 0)
	{
		if (strcmp(hd->method, "POST"))
		{
			send_error(cptr, 501, "Method Not Implemented", TRUE);
			sendq_add_eof(cptr);
			return;
		}
		if (hd->length <= 0)
		{
			send_error(cptr, 411, "Length Required", TRUE);
			sendq_add_eof(cptr);
			return;
		}
		if (hd->length > REQUEST_MAX)
		{
			send_error(cptr, 413, "Request Entity Too Large", TRUE);
			sendq_add_eof(cptr);
			return;
		}
		if (!hd->correct_content_type)
		{
			send_error(cptr, 415, "Unsupported Media Type", TRUE);
			sendq_add_eof(cptr);
			return;
		}
		if (hd->expect_100_continue)
		{
			snprintf(outbuf, sizeof outbuf,
					"HTTP/1.1 100 Continue\r\nServer: Atheme/%s\r\n\r\n",
					version);
			sendq_add(cptr, outbuf, strlen(outbuf));
		}
		hd->requestbuf = smalloc(hd->length + 1);
	}
	else
		process_header(cptr, buf);
}

static void httpd_closehandler(connection_t *cptr)
{
	struct httpddata *hd;

	slog(LG_DEBUG, "httpd_closehandler(): fd %d (%s) closed", cptr->fd, cptr->hbuf);
	hd = cptr->userdata;
	if (hd != NULL)
	{
		free(hd->requestbuf);
		free(hd);
	}
	cptr->userdata = NULL;
}

static void do_listen(connection_t *cptr)
{
	connection_t *newptr;
	struct httpddata *hd;

	newptr = connection_accept_tcp(cptr, recvq_put, sendq_flush);
	slog(LG_DEBUG, "do_listen(): accepted httpd from %s fd %d", newptr->hbuf, newptr->fd);
	hd = smalloc(sizeof(*hd));
	hd->requestbuf = NULL;
	hd->replybuf = NULL;
	hd->connection_close = FALSE;
	clear_httpddata(hd);
	newptr->userdata = hd;
	newptr->recvq_handler = httpd_recvqhandler;
	newptr->close_handler = httpd_closehandler;
}

extern list_t connection_list; /* XXX ? */

static void httpd_checkidle(void *arg)
{
	node_t *n;
	connection_t *cptr;

	(void)arg;
	LIST_FOREACH(n, connection_list.head)
	{
		cptr = n->data;
		if (cptr->listener == listener && cptr->last_recv + 300 < CURRTIME)
		{
			if (sendq_nonempty(cptr))
				cptr->last_recv = CURRTIME;
			else
				/* from a timeout function,
				 * connection_close_soon() may take quite
				 * a while, and connection_close() is safe
				 * -- jilles */
				connection_close(cptr);
		}
	}
}

static char *dump_buffer(char *buf, int length)
{
	struct httpddata *hd;
	char buf1[300];

	hd = current_cptr->userdata;
	snprintf(buf1, sizeof buf1, "HTTP/1.1 200 OK\r\n"
			"%s"
			"Server: Atheme/%s\r\n"
			"Content-Type: text/xml\r\n"
			"Content-Length: %d\r\n\r\n",
			hd->connection_close ? "Connection: close\r\n" : "",
			version, length);
	sendq_add(current_cptr, buf1, strlen(buf1));
	sendq_add(current_cptr, buf, length);
	if (hd->connection_close)
		sendq_add_eof(current_cptr);
}

static void xmlrpc_config_ready(void *vptr)
{
	if (xmlrpc_config.host != NULL && xmlrpc_config.port != 0)
	{
		listener = connection_open_listener_tcp(xmlrpc_config.host,
			xmlrpc_config.port, do_listen);
		hook_del_hook("config_ready", xmlrpc_config_ready);
	}
	else
		slog(LG_ERROR, "xmlrpc_config_ready(): xmlrpc{} block missing or invalid");
}

void _modinit(module_t *m)
{
	event_add("httpd_checkidle", httpd_checkidle, NULL, 60);

	/* This module needs a rehash to initialize fully if loaded
	 * at run time */
	hook_add_event("config_ready");
	hook_add_hook("config_ready", xmlrpc_config_ready);

	add_top_conf("XMLRPC", conf_xmlrpc);
	add_conf_item("HOST", &conf_xmlrpc_table, conf_xmlrpc_host);
	add_conf_item("PORT", &conf_xmlrpc_table, conf_xmlrpc_port);

	xmlrpc_set_buffer(dump_buffer);
	xmlrpc_set_options(XMLRPC_HTTP_HEADER, XMLRPC_OFF);
	xmlrpc_register_method("atheme.login", xmlrpcmethod_login);
	xmlrpc_register_method("atheme.logout", xmlrpcmethod_logout);
	xmlrpc_register_method("atheme.command", xmlrpcmethod_command);
}

void _moddeinit(void)
{
	event_delete(httpd_checkidle, NULL);
	connection_close_soon_children(listener);
	xmlrpc_unregister_method("atheme.login");
	xmlrpc_unregister_method("atheme.logout");
	xmlrpc_unregister_method("atheme.command");
	del_conf_item("HOST", &conf_xmlrpc_table);
	del_conf_item("PORT", &conf_xmlrpc_table);
	del_top_conf("XMLRPC");
}

static void xmlrpc_command_fail(sourceinfo_t *si, faultcode_t code, const char *message)
{
	connection_t *cptr;
	struct httpddata *hd;
	char newmessage[256];
	int i;
	const char *p;

	cptr = si->connection;
	hd = cptr->userdata;
	if (hd->sent_reply)
		return;
	i = 0, p = message;
	while (i < 255 && *p != '\0')
		if (*p > '\0' && *p < ' ')
			p++;
		else
			newmessage[i++] = *p, p++;
	newmessage[i] = '\0';
	xmlrpc_generic_error(code, newmessage);
	hd->sent_reply = TRUE;
}

static void xmlrpc_command_success_nodata(sourceinfo_t *si, const char *message)
{
	connection_t *cptr;
	struct httpddata *hd;
	char buf[XMLRPC_BUFSIZE];
	char *p;
	const char *q;

	cptr = si->connection;
	hd = cptr->userdata;
	if (hd->sent_reply)
		return;
	if (hd->replybuf != NULL)
	{
		hd->replybuf = srealloc(hd->replybuf, strlen(hd->replybuf) + strlen(message) + 1);
		p = hd->replybuf + strlen(hd->replybuf);
		*p++ = '\n';
	}
	else
	{
		hd->replybuf = smalloc(strlen(message));
		p = hd->replybuf;
	}
	q = message;
	while (*q != '\0')
		if (*q > '\0' && *q < ' ')
			q++;
		else
			*p++ = *q++;
	*p = '\0';
}

static void xmlrpc_command_success_string(sourceinfo_t *si, const char *result, const char *message)
{
	connection_t *cptr;
	struct httpddata *hd;
	char buf[XMLRPC_BUFSIZE];

	cptr = si->connection;
	hd = cptr->userdata;
	if (hd->sent_reply)
		return;
	xmlrpc_string(buf, result);
	xmlrpc_send(1, buf);
	hd->sent_reply = TRUE;
}

/* These taken from the old modules/xmlrpc/account.c */
/*
 * atheme.login
 *
 * XML Inputs:
 *       account name, password, source ip (optional)
 *
 * XML Outputs:
 *       fault 1 - insufficient parameters
 *       fault 3 - account is not registered
 *       fault 5 - invalid username and password
 *       fault 6 - account is frozen
 *       default - success (authcookie)
 *
 * Side Effects:
 *       an authcookie ticket is created for the myuser_t.
 *       the user's lastlogin is updated
 */
static int xmlrpcmethod_login(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	authcookie_t *ac;
	char buf[BUFSIZE];
	const char *sourceip;

	if (parc < 2)
	{
		xmlrpc_generic_error(1, "Insufficient parameters.");
		return 0;
	}

	sourceip = parc >= 3 && *parv[2] != '\0' ? parv[2] : NULL;

	if (!(mu = myuser_find(parv[0])))
	{
		xmlrpc_generic_error(3, "The account is not registered.");
		return 0;
	}

	if (metadata_find(mu, METADATA_USER, "private:freeze:freezer") != NULL)
	{
		logcommand_external(nicksvs.me, "xmlrpc", conn, sourceip, NULL, CMDLOG_LOGIN, "failed LOGIN to %s (frozen)", mu->name);
		xmlrpc_generic_error(6, "The account has been frozen.");
		return 0;
	}

	if (!verify_password(mu, parv[1]))
	{
		logcommand_external(nicksvs.me, "xmlrpc", conn, sourceip, NULL, CMDLOG_LOGIN, "failed LOGIN to %s (bad password)", mu->name);
		xmlrpc_generic_error(5, "The password is not valid for this account.");
		return 0;
	}

	mu->lastlogin = CURRTIME;

	ac = authcookie_create(mu);

	logcommand_external(nicksvs.me, "xmlrpc", conn, sourceip, mu, CMDLOG_LOGIN, "LOGIN");

	xmlrpc_string(buf, ac->ticket);
	xmlrpc_send(1, buf);

	return 0;
}

/*
 * atheme.logout
 *
 * XML inputs:
 *       authcookie, and account name.
 *
 * XML outputs:
 *       fault 1 - insufficient parameters
 *       fault 3 - unknown user
 *       fault 5 - validation failed
 *       default - success message
 *
 * Side Effects:
 *       an authcookie ticket is destroyed.
 */
static int xmlrpcmethod_logout(void *conn, int parc, char *parv[])
{
	authcookie_t *ac;
	myuser_t *mu;
	char buf[XMLRPC_BUFSIZE];

	if (parc < 2)
	{
		xmlrpc_generic_error(1, "Insufficient parameters.");
		return 0;
	}

	if ((mu = myuser_find(parv[1])) == NULL)
	{
		xmlrpc_generic_error(3, "Unknown user.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(5, "Invalid authcookie for this account.");
		return 0;
	}

	logcommand_external(nicksvs.me, "xmlrpc", conn, NULL, mu, CMDLOG_LOGIN, "LOGOUT");

	ac = authcookie_find(parv[0], mu);
	authcookie_destroy(ac);

	xmlrpc_string(buf, "You are now logged out.");
	xmlrpc_send(1, buf);

	return 0;
}

/*
 * atheme.command
 *
 * XML inputs:
 *       authcookie, account name, source ip, service name, command name,
 *       parameters.
 *
 * XML outputs:
 *       depends on command
 *
 * Side Effects:
 *       command is executed
 */
static int xmlrpcmethod_command(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	service_t *svs;
	command_t *cmd;
	sourceinfo_t si;
	int newparc;
	char *newparv[20];
	struct httpddata *hd = ((connection_t *)conn)->userdata;
	char buf[XMLRPC_BUFSIZE];
	int i;

	for (i = 0; i < parc; i++)
	{
		if (strchr(parv[i], '\r') || strchr(parv[i], '\n'))
		{
			xmlrpc_generic_error(2, "Invalid parameters.");
			return 0;
		}
	}

	if (parc < 5)
	{
		xmlrpc_generic_error(1, "Insufficient parameters.");
		return 0;
	}

	if (*parv[1] != '\0' && strlen(parv[0]) > 1)
	{
		if ((mu = myuser_find(parv[1])) == NULL)
		{
			xmlrpc_generic_error(3, "Unknown user.");
			return 0;
		}

		if (authcookie_validate(parv[0], mu) == FALSE)
		{
			xmlrpc_generic_error(5, "Invalid authcookie for this account.");
			return 0;
		}
	}
	else
		mu = NULL;

	svs = find_service(parv[3]);
	if (svs == NULL || svs->cmdtree == NULL)
	{
		slog(LG_DEBUG, "xmlrpcmethod_command(): invalid service %s", parv[3]);
		xmlrpc_generic_error(3, "Invalid service name.");
		return 0;
	}
	cmd = command_find(svs->cmdtree, parv[4]);
	if (cmd == NULL)
	{
		xmlrpc_generic_error(3, "Invalid command name.");
		return 0;
	}

	memset(newparv, '\0', sizeof newparv);
	newparc = parc - 5;
	if (newparc > 20)
		newparc = 20;
	if (newparc > 0)
		memcpy(newparv, parv + 5, newparc * sizeof(parv[0]));
	memset(&si, '\0', sizeof si);
	si.smu = mu;
	si.service = svs;
	/*si.realip = parv[2];*/
	si.connection = conn;
	si.v = &xmlrpc_vtable;
	command_exec(svs, &si, cmd, newparc, newparv);
	if (!hd->sent_reply)
	{
		if (hd->replybuf != NULL)
		{
			xmlrpc_string(buf, hd->replybuf);
			xmlrpc_send(1, buf);
		}
		else
			xmlrpc_generic_error(14, "Command did not return a result.");
	}

	return 0;
}
