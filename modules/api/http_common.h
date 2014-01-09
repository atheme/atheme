#ifndef ATHEME_HTTP_COMMON_H
#define ATHEME_HTTP_COMMON_H

#include "atheme.h"

typedef struct http_client_ http_client_t;

typedef void (*http_cb_t)(http_client_t *http, void *userdata);

struct http_client_
{
	char *url;
	char *uri;
	char *protocol;
	char *domain;
	char *query_string;
	int port;
	int http_code;

	int error;
	char *error_str;

	size_t data_len;
	char *data;
	mowgli_patricia_t *headers;

	int fd;
	http_cb_t callback;
	void *userdata;

	size_t query_string_len;
	size_t query_string_size;
};

#endif /* ATHEME_HTTP_COMMON_H */
