/*
 * Copyright (c) 2006 Jilles Tjoelker <jilles -at- stack.nl>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A simple web server
 *
 */

#include "atheme.h"
#include "datastream.h"

/*#define WWW_ROOT (SHAREDIR "/help")*/
#define WWW_ROOT "/home/jilles/src/svn/atheme/tools/htmlhelp"

DECLARE_MODULE_V1
(
	"contrib/gen_httpd", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Jilles Tjoelker <jilles -at- stack.nl>"
);

connection_t *listener;

struct httpddata
{
	char method[64];
	char filename[256];
	bool connection_close;
};

static int open_file(const char *filename)
{
	char fname[256];

	if (strstr(filename, ".."))
		return -1;
	if (!strcmp(filename, "/"))
		filename = "/index.html";
	snprintf(fname, sizeof fname, "%s/%s", WWW_ROOT, filename);
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
	struct httpddata *hd;
	char buf1[300];
	char buf2[700];

	hd = cptr->userdata;
	if (errorcode < 100 || errorcode > 999)
		errorcode = 500;
	snprintf(buf2, sizeof buf2, "HTTP/1.1 %d %s\r\n", errorcode, text);
	snprintf(buf1, sizeof buf1, "HTTP/1.1 %d %s\r\n"
			"Server: Atheme/%s\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: %d\r\n\r\n%s",
			errorcode, text, version, (int)strlen(buf2),
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
	bool is_get;

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
	hd = cptr->userdata;
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
		is_get = !strcmp(hd->method, "GET");
		if (!is_get && strcmp(hd->method, "HEAD"))
		{
			send_error(cptr, 501, "Method Not Implemented", true);
			check_close(cptr);
			return;
		}
		hd->method[0] = '\0';
		in = open_file(hd->filename);
		if (in == -1 || fstat(in, &sb) == -1 || !S_ISREG(sb.st_mode))
		{
			if (in != -1)
				close(in);
			slog(LG_INFO, "httpd_recvqhandler(): 404 for %s", hd->filename);
			send_error(cptr, 404, "Not Found", is_get);
			check_close(cptr);
			return;
		}
		slog(LG_INFO, "httpd_recvqhandler(): 200 for %s", hd->filename);
		snprintf(outbuf, sizeof outbuf,
				"HTTP/1.1 200 OK\r\nServer: Atheme/%s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n\r\n",
				version,
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
		process_header(cptr, buf);
}

static void httpd_closehandler(connection_t *cptr)
{
	struct httpddata *hd;

	slog(LG_DEBUG, "httpd_closehandler(): fd %d (%s) closed", cptr->fd, cptr->hbuf);
	hd = cptr->userdata;
	free(hd);
	cptr->userdata = NULL;
}

static void do_listen(connection_t *cptr)
{
	connection_t *newptr;
	struct httpddata *hd;

	newptr = connection_accept_tcp(cptr, recvq_put, NULL);
	slog(LG_DEBUG, "do_listen(): accepted httpd from %s fd %d", newptr->hbuf, newptr->fd);
	hd = smalloc(sizeof(*hd));
	hd->method[0] = '\0';
	hd->filename[0] = '\0';
	hd->connection_close = false;
	newptr->userdata = hd;
	newptr->recvq_handler = httpd_recvqhandler;
	newptr->close_handler = httpd_closehandler;
}

static void httpd_checkidle(void *arg)
{
	mowgli_node_t *n;
	connection_t *cptr;

	(void)arg;
	if (listener == NULL)
		return;
	MOWGLI_ITER_FOREACH(n, connection_list.head)
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

void _modinit(module_t *m)
{
	listener = connection_open_listener_tcp("127.0.0.1", 7100, do_listen);
	if (listener == NULL)
		slog(LG_ERROR, "gen_httpd/_modinit(): failed to open listener on host %s port %d", "127.0.0.1", 7100);
	event_add("httpd_checkidle", httpd_checkidle, NULL, 60);
}

void _moddeinit(void)
{
	event_delete(httpd_checkidle, NULL);
	connection_close_soon_children(listener);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
