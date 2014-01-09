#include "http_common.h"
#include "datastream.h"

DECLARE_MODULE_V1(
	"api/http", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

void _modinit(module_t *m)
{
}

void _moddeinit(module_unload_intent_t intent)
{
}

http_client_t *http_client_new(void)
{
	http_client_t *h;

	h = smalloc(sizeof(http_client_t));
	h->headers = mowgli_patricia_create(strcasecanon);

	h->query_string_size = 32;
	h->query_string_len = 0;
	h->query_string = smalloc(h->query_string_size);

	return h;
}

static void query_string_reset(http_client_t *http)
{
	http->query_string_len = 0;
	http->query_string[0] = '\0';
}

static void query_string_concat(http_client_t *http, char *str, size_t len)
{
	if (http->query_string_size <= http->query_string_len + len) {
		http->query_string_size *= 2;
		if (http->query_string_size <= http->query_string_len + len)
			http->query_string_size = http->query_string_len + len + 10;
		http->query_string = srealloc(http->query_string,
		                              http->query_string_size);
	}

	memcpy(http->query_string + http->query_string_len, str, len);
	http->query_string_len += len;
	http->query_string[http->query_string_len] = '\0';
}

void http_parse_headers(http_client_t *container, const char *http_response)
{
	const char *buffer = http_response;

	while(1)
	{
		char *name = NULL;
		char *value = NULL;
		const char *sep = NULL;
		const char *space = NULL;
		size_t name_size, value_size;

		int header_length = strcspn(buffer, "\n\0");
		++header_length; // include the \n or \0

		// Special-case the HTTP response code
		if(strncmp("HTTP/", buffer, 5) == 0)
		{
			char proto[4];
			char status[4];

			strncpy(proto, buffer + 5, 3);
			proto[3] = '\0';
			strncpy(status, buffer + 9, 3);
			status[3] = '\0';

			container->http_code = strtol(status, NULL, 10);

			buffer += header_length;
			continue;
		}

		// A blank newline after a header indicates payload follows
		if(*buffer == '\r' || *buffer == '\n')
		{
			buffer += header_length;
			break;
		}
		if(*(buffer + header_length) == '\0') break;

		sep = strchr(buffer, ':');
		if(sep == NULL)
		{
			buffer += header_length;
			continue;
		}
		space = strchr(sep, ' ');
		// The space after the colon is not *explicitly* required by HTTP/1.1 standards
		if(space == NULL) space = sep + 1;

		name_size = (sep - buffer);
		name = (char *)scalloc(name_size + 1, 1);

		value_size = (header_length - (space + 1 /* no space */ - buffer));
		value = (char *)scalloc(value_size + 1, 1);

		strncpy(name, buffer, name_size);
		name[name_size] = '\0';
		strncpy(value, space + 1 /* no space */, value_size);
		value[value_size] = '\0';

		mowgli_patricia_add(container->headers, name, value);

		free(name);

		buffer += header_length;
	}

	container->data = (char *)buffer;
	return;
}

int http_parse_url(http_client_t *container, const char *url)
{
	unsigned int proto_length;
	char *port_str;
	int domain_length;
	unsigned int nextchar;

	// Get the protocol
	container->protocol = (char *)scalloc(5, sizeof(char));
	proto_length = strcspn(url, "://");
	if(proto_length == strlen(url))
	{
		// No protocol, assume HTTP
		strncpy(container->protocol, "http\0", 5);
	}
	else
	{
		container->protocol = (char *)srealloc(container->protocol,
				sizeof(char) * (proto_length + 1));
		strncpy(container->protocol, url, proto_length);
		container->protocol[proto_length] = '\0';
		url += (proto_length + 3);  // skip proto and ://
	}

	for (nextchar = 0; nextchar < proto_length; nextchar++)
		container->protocol[nextchar] = (char)tolower(container->protocol[nextchar]);

	if (strcmp("https", container->protocol) == 0) {
		container->port = 443;
	} else if (strcmp("http", container->protocol) == 0) {
		container->port = 80;
	} else if (strcmp("ftp", container->protocol) == 0) {
		container->port = 21;
	} else if (strcmp("gopher", container->protocol) == 0) {
		container->port = 70;
	} else {
		container->error = 1;
		container->error_str = sstrdup("unrecognised protocol");
		free(container->protocol);
		return 1;
	}

	domain_length = strcspn(url, "/");
	container->domain = (char *)scalloc(domain_length + 1, sizeof(char));
	strncpy(container->domain, url, domain_length);
	container->domain[domain_length] = '\0';
	if (container->domain[0] == '[')
	{
		//IPv6 address.
		port_str = strrchr(container->domain, (int)':');
		//ensure this isn't the last oct of a v6 address
		if (strchr(port_str, (int)']') == NULL)
		{
			int temp_port;
			temp_port = (int)strtol((char *)(port_str + 1), NULL, 10);
			if (temp_port > 65535)
			{
				container->error = 2;
				container->error_str = sstrdup("Invalid port number. Using default");
			}
			else
			{
				container->port = temp_port;
			}
			port_str[0] = '\0';
		}
	}
	else
	{
		port_str = strchr(container->domain, (int)':');
		if (port_str != NULL)
		{
			int temp_port;
			temp_port = atoi((char *)(port_str + 1));
			if (temp_port > 65535)
			{
				container->error = 2;
				container->error_str = sstrdup("Invalid port number. Using default");
			}
			else
			{
				container->port = temp_port;
			}
			port_str[0] = '\0';
		}
	}

	if ((unsigned int)domain_length == strlen(url))
	{
		container->uri = (char *)scalloc(2, sizeof(char));
		container->uri[0] = '/'; container->uri[1] = '\0';
	}
	else
	{
		size_t length = strlen(url) - domain_length;
		container->uri = (char *)scalloc(length + 1, sizeof(char));
		strncpy(container->uri, (url + domain_length), length);
		container->uri[length] = '\0';
		char *params = strchr(container->uri, '?');
		if (params != NULL)
		{
			query_string_reset(container);
			query_string_concat(container, params, strlen(params));
		}
	}

	return true;
}

void http_add_param(http_client_t *http, const char *key, const char *value)
{
	size_t len = strlen(key) + strlen(value) + 3;
	char tmp[len];

	snprintf(tmp, len, "%c%s=%s",
	         http->query_string[0] ? '?' : '&', key, value);

	query_string_concat(http, tmp, len);
}

void http_write_GET(connection_t *cptr)
{
	mowgli_node_t *n, *tn;
	http_client_t *container = NULL;
	char buf[BUFSIZE];
	size_t len;

	if (!(container = cptr->userdata))
		return;

	len = snprintf(buf, BUFSIZE,
			"GET %s%s HTTP/1.1\r\n"
			"User-Agent: Atheme/%s\r\n"
			"Host: %s\r\n"
			"Accept: */*\r\n"
			"\r\n",
			container->uri,
			container->query_string,
			PACKAGE_VERSION,
			container->domain);
	sendq_add(cptr, buf, len);
}

void http_handle_EOF(connection_t *cptr)
{
	int i, l;
	char *buf;
	mowgli_node_t *n, *tn;
	http_client_t *container;

	if (!(container = cptr->userdata))
	{
		slog(LG_INFO, "eof() couldn't find http client");
		return;
	}

	l = recvq_length(cptr);
	buf = smalloc(l);
	recvq_get(cptr, buf, l);
	http_parse_headers(container, buf);
	container->callback(container, container->userdata);
}

int http_get(http_client_t *container, void *userdata, http_cb_t cb)
{
	connection_t *cptr;

	container->callback = cb;
	container->userdata = userdata;

	cptr = connection_open_tcp(container->domain,
			NULL, container->port, recvq_put, http_write_GET);
	container->fd = cptr->fd;
	/*container->cptr = cptr;*/
	cptr->close_handler = http_handle_EOF;
	cptr->userdata = container;
}
