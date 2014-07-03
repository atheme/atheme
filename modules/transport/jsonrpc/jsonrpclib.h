/*
 * Copyright (c) 2014 Atheme Development Group
 *
 * JSONRPC library header
 *
 */

#ifndef JSONRPC_H
#define JSONRPC_H

#include "atheme.h"

typedef bool (*jsonrpc_method_t)(void *conn, mowgli_list_t *params, char *id);

typedef struct {
    sourceinfo_t *base;
    char *id;
} jsonrpc_sourceinfo_t;

E char *jsonrpc_normalizeBuffer(const char *buf);

E jsonrpc_method_t get_json_method(const char *method_name);

E void jsonrpc_process(char *buffer, void *userdata);
E void jsonrpc_register_method(const char *method_name, bool (*method)(void *conn, mowgli_list_t *params, char *id));
E void jsonrpc_unregister_method(const char *method_name);
E void jsonrpc_send_data(void *conn, char *str);
E void jsonrpc_success_string(void *conn, const char *str, const char *id);
E void jsonrpc_failure_string(void *conn, int code, const char *str, const char *id);

#endif
