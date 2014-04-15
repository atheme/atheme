/*
 * Copyright (c) 2014 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * URI parsing code borrowed from libAmy <https://github.com/wilcox-tech/eScape>,
 * licensed under NCSA.
 *
 */

#ifndef ATHEME_HTTP_H
#define ATHEME_HTTP_H

#include "http_common.h"

static http_client_t * (*http_client_new)(void);
static void (*http_parse_headers)(http_client_t *container, const char *http_response);
static int (*http_parse_url)(http_client_t *container, const char *url);
static void (*http_add_param)(http_client_t *http, const char *key, const char *value);
//int (*http_get)(const char *url, http_cb_t cb, void *userdata);
static int (*http_get)(http_client_t *container, void *userdata, http_cb_t cb);

static inline void use_http_symbols(module_t *m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "api/http");
	MODULE_TRY_REQUEST_SYMBOL(m, http_client_new, "api/http", "http_client_new");
	MODULE_TRY_REQUEST_SYMBOL(m, http_parse_headers, "api/http", "http_parse_headers");
	MODULE_TRY_REQUEST_SYMBOL(m, http_parse_url, "api/http", "http_parse_url");
	MODULE_TRY_REQUEST_SYMBOL(m, http_add_param, "api/http", "http_add_param");
	MODULE_TRY_REQUEST_SYMBOL(m, http_get, "api/http","http_get");
}

#endif /* ATHEME_HTTP_H */
