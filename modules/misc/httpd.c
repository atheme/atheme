/*
 * Copyright (c) 2005-2007 Jilles Tjoelker et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Generic HTTPd
 *
 */

#include "atheme.h"
#include "httpd.h"
#include "datastream.h"

#define REQUEST_MAX 65536 /* maximum size of one call */

DECLARE_MODULE_V1
(
	"misc/httpd", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

connection_t *listener;
mowgli_list_t httpd_path_handlers;

/* conf stuff */
mowgli_list_t conf_httpd_table;
struct httpd_configuration
{
	char *host;
	char *www_root;
	unsigned int port;
} httpd_config;

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
	hd->correct_content_type = false;
	hd->expect_100_continue = false;
	hd->sent_reply = false;
}

static int open_file(const char *filename)
{
	char fname[256];

	if (strstr(filename, ".."))
		return -1;
	if (!strcmp(filename, "/"))
		filename = "/index.html";
	snprintf(fname, sizeof fname, "%s/%s", httpd_config.www_root, filename);
	return open(fname, O_RDONLY);
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
		while (p != NULL)
		{
			if (!strcasecmp(p, "close"))
			{
				slog(LG_DEBUG, "process_header(): Connection: close requested by fd %d", cptr->fd);
				hd->connection_close = true;
			}
			p = strtok(NULL, ", \t");
		}
	}
	else if (!strcasecmp(line, "Content-Length"))
	{
		hd->length = atoi(p);
	}
	else if (!strcasecmp(line, "Content-Type"))
	{
		p = strtok(p, "; \t");
		hd->correct_content_type = p != NULL && (!strcasecmp(p, "text/xml") || !strcasecmp(p, "application/json"));
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

static void send_error(connection_t *cptr, int errorcode, const char *text, bool sendentity)
{
	char buf1[300];
	char buf2[700];

	if (errorcode < 100 || errorcode > 999)
		errorcode = 500;
	snprintf(buf2, sizeof buf2, "HTTP/1.1 %d %s\r\n", errorcode, text);
	snprintf(buf1, sizeof buf1, "HTTP/1.1 %d %s\r\n"
			"Server: Atheme/%s\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: %lu\r\n\r\n%s",
			errorcode, text, PACKAGE_VERSION, (unsigned long)strlen(buf2),
			sendentity ? buf2 : "");
	sendq_add(cptr, buf1, strlen(buf1));
}

static const char *content_type(const char *filename)
{
	const char *p;

	if (!strcmp(filename, "/"))
		return "text/html";

	p = strrchr(filename, '.');
	if (p == NULL)
		return "text/plain";
	p++;
	if (!strcasecmp(p, "html") || !strcasecmp(p, "htm"))
		return "text/html";
	else if (!strcasecmp(p, "txt"))
		return "text/plain";
	else if (!strcasecmp(p, "jpg") || !strcasecmp(p, "jpeg"))
		return "image/jpeg";
	else if (!strcasecmp(p, "gif"))
		return "image/gif";
	else if (!strcasecmp(p, "png"))
		return "image/png";
	return "application/octet-stream";
}

static void httpd_recvqhandler(connection_t *cptr)
{
	char buf[BUFSIZE * 2];
	char outbuf[BUFSIZE * 2];
	int count;
	struct httpddata *hd;
	char *p;
	int in;
	struct stat sb;
	off_t count1;
	mowgli_node_t *n;
	path_handler_t *ph = NULL;
	bool is_get, is_post, handling_done = false;

	hd = cptr->userdata;

	MOWGLI_ITER_FOREACH(n, httpd_path_handlers.head)
	{
		ph = (path_handler_t *)n->data;
		handling_done = !strcmp(hd->filename, ph->path);

		if (handling_done)
			break;
	}

	if (handling_done)
	{
		if (hd->requestbuf != NULL)
		{
			count = recvq_get(cptr, hd->requestbuf + hd->lengthdone, hd->length - hd->lengthdone);
			if (count <= 0)
				return;
			hd->lengthdone += count;
			if (hd->lengthdone != hd->length)
				return;
			hd->requestbuf[hd->length] = '\0';
			
			ph->handler(cptr, hd->requestbuf);

			clear_httpddata(hd);
			return;  
		}
	}

	count = recvq_getline(cptr, buf, sizeof buf - 1);
	if (count <= 0)
		return;
	if (cptr->flags & CF_NONEWLINE)
	{
		slog(LG_INFO, "httpd_recvqhandler(): throwing out fd %d (%s) for excessive line length", cptr->fd, cptr->hbuf);
		send_error(cptr, 400, "Bad request", true);
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
			hd->connection_close = true;
		slog(LG_DEBUG, "httpd_recvqhandler(): request %s for %s", hd->method, hd->filename);
	}
	else if (count == 0)
	{
		is_get  = !strcmp(hd->method, "GET");
		is_post = !strcmp(hd->method, "POST");

		if (!is_post && !is_get)
		{
			send_error(cptr, 501, "Method Not Implemented", true);
			sendq_add_eof(cptr);
			return;
		}

		hd->method[0] = '\0';

		if (!handling_done)
		{
			in = open_file(hd->filename);
			if (in == -1 || fstat(in, &sb) == -1 || !S_ISREG(sb.st_mode))
			{
				if (in != -1)
					close(in);
				slog(LG_DEBUG, "httpd_recvqhandler(): 404 for \2%s\2", hd->filename);
				send_error(cptr, 404, "Not Found", is_get);
				check_close(cptr);
				return;
			}
			slog(LG_INFO, "httpd_recvqhandler(): 200 for %s", hd->filename);
			snprintf(outbuf, sizeof outbuf,
					"HTTP/1.1 200 OK\r\nServer: Atheme/%s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n\r\n",
					PACKAGE_VERSION,
					content_type(hd->filename),
					(unsigned long)sb.st_size);
			sendq_add(cptr, outbuf, strlen(outbuf));
			count1 = is_get ? sb.st_size : 0;
			while (count1 > 0)
			{
				count = sizeof outbuf;
				if (count > count1)
					count = count1;
				count = read(in, outbuf, count);
				if (count <= 0)
					break;
				sendq_add(cptr, outbuf, count);
				count1 -= count;
			}
			close(in);
			if (count1 > 0)
			{
				slog(LG_INFO, "httpd_recvqhandler(): disconnecting fd %d (%s), read failed on %s", cptr->fd, cptr->hbuf, hd->filename);
				cptr->flags |= CF_DEAD;
			}
			else
				check_close(cptr);
		}
		else
		{
			if (hd->length <= 0)
			{
				send_error(cptr, 411, "Length Required", true);
				sendq_add_eof(cptr);
				return;
			}
			if (hd->length > REQUEST_MAX)
			{
				send_error(cptr, 413, "Request Entity Too Large", true);
				sendq_add_eof(cptr);
				return;
			}
			if (!hd->correct_content_type)
			{
				send_error(cptr, 415, "Unsupported Media Type", true);
				sendq_add_eof(cptr);
				return;
			}
			if (hd->expect_100_continue)
			{
				snprintf(outbuf, sizeof outbuf,
						"HTTP/1.1 100 Continue\r\nServer: Atheme/%s\r\n\r\n",
						PACKAGE_VERSION);
				sendq_add(cptr, outbuf, strlen(outbuf));
			}
			hd->requestbuf = smalloc(hd->length + 1);
		}
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

	newptr = connection_accept_tcp(cptr, recvq_put, NULL);
	slog(LG_DEBUG, "do_listen(): accepted httpd from %s fd %d", newptr->hbuf, newptr->fd);
	hd = smalloc(sizeof(*hd));
	hd->requestbuf = NULL;
	hd->replybuf = NULL;
	hd->connection_close = false;
	clear_httpddata(hd);
	newptr->userdata = hd;
	newptr->recvq_handler = httpd_recvqhandler;
	newptr->close_handler = httpd_closehandler;
}

static void httpd_checkidle(void *arg)
{
	mowgli_node_t *n, *tn;
	connection_t *cptr;

	(void)arg;
	if (listener == NULL)
		return;
	MOWGLI_ITER_FOREACH_SAFE(n, tn, connection_list.head)
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

static void httpd_config_ready(void *vptr)
{
	if (httpd_config.host != NULL && httpd_config.port != 0)
	{
		/* Some code depends on connection_t.listener == listener. */
		if (listener != NULL)
			return;
		listener = connection_open_listener_tcp(httpd_config.host,
			httpd_config.port, do_listen);
		if (listener == NULL)
			slog(LG_ERROR, "httpd_config_ready(): failed to open listener on host %s port %d", httpd_config.host, httpd_config.port);
	}
	else
		slog(LG_ERROR, "httpd_config_ready(): httpd {} block missing or invalid");
}

void _modinit(module_t *m)
{
	event_add("httpd_checkidle", httpd_checkidle, NULL, 60);

	/* This module needs a rehash to initialize fully if loaded
	 * at run time */
	hook_add_event("config_ready");
	hook_add_config_ready(httpd_config_ready);

	add_subblock_top_conf("HTTPD", &conf_httpd_table);
	add_dupstr_conf_item("HOST", &conf_httpd_table, 0, &httpd_config.host, NULL);
	add_dupstr_conf_item("WWW_ROOT", &conf_httpd_table, 0, &httpd_config.www_root, NULL);
	add_uint_conf_item("PORT", &conf_httpd_table, 0, &httpd_config.port, 1, 65535, 0);
}

void _moddeinit(void)
{
	event_delete(httpd_checkidle, NULL);
	hook_del_config_ready(httpd_config_ready);
	connection_close_soon_children(listener);
	del_conf_item("HOST", &conf_httpd_table);
	del_conf_item("WWW_ROOT", &conf_httpd_table);
	del_conf_item("PORT", &conf_httpd_table);
	del_top_conf("HTTPD");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
