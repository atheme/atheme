/*
 * Copyright (c) 2005 Atheme Development Group
 *                and Trystan Scott Lee <trystan@nomadirc.net>
 *
 * JSONRPC library header, hacked up for Atheme.
 *
 * $Id: jsonrpc.h 7779 2007-03-03 13:55:42Z pippijn $
 */
#ifndef JSONRPC_H
#define JSONRPC_H

#include "atheme.h"

#define stricmp strcasecmp

#define JSONRPC_STOP 1
#define JSONRPC_CONT 0

#define JSONRPC_ERR_OK			0
#define JSONRPC_ERR_MEMORY		1
#define JSONRPC_ERR_PARAMS		2
#define JSONRPC_ERR_EXISTS		3
#define JSONRPC_ERR_NOEXIST		4
#define JSONRPC_ERR_NOUSER		5
#define JSONRPC_ERR_NOLOAD      6
#define JSONRPC_ERR_NOUNLOAD    7
#define JSONRPC_ERR_SYNTAX		8
#define JSONRPC_ERR_NODELETE	9
#define JSONRPC_ERR_UNKNOWN		10
#define JSONRPC_ERR_FILE_IO     11
#define JSONRPC_ERR_NOSERVICE   12
#define JSONRPC_ERR_NO_MOD_NAME 13

/*
 * Header that defines much of the json/jsonrpc library
 */

#define JSONRPC_BUFSIZE         1024
#define JSONLIB_VERSION		 "1.0.0"
#define JSONLIB_AUTHOR		 "Trystan Scott Lee <trystan@nomadirc.net>"

#define JSONRPCCMD JSONRPCCMD_cmdTable
#define CMD_HASH(x)      (((x)[0]&31)<<5 | ((x)[1]&31))	/* Will gen a hash from a string :) */
#define MAX_CMD_HASH 1024

#define JSONRPC_ON "on"
#define JSONRPC_OFF "off"

#define JSONRPC_I4 "i4"
#define JSONRPC_INT "integer"

#define JSONRPC_HTTP_HEADER 1
#define JSONRPC_ENCODE 2
#define JSONRPC_INTTAG 3

typedef int (*JSONRPCMethodFunc)(void *userdata, int ac, char **av);

typedef struct JSONRPCCmd_ JSONRPCCmd;
typedef struct JSONRPCCmdHash_ JSONRPCCmdHash;

extern JSONRPCCmdHash *JSONRPCCMD[MAX_CMD_HASH];

struct JSONRPCCmd_ {
    JSONRPCMethodFunc func;
	char *name;
    int core;
    char *mod_name;
    JSONRPCCmd *next;
};

struct JSONRPCCmdHash_ {
        char *name;
        JSONRPCCmd *json;
        JSONRPCCmdHash *next;
};

typedef struct jsonrpc_settings {
	char *(*setbuffer)(char *buffer, int len);
	char *encode;
    int httpheader;
    char *inttagstart;
	char *inttagend;
} JSONRPCSet;

E int jsonrpc_getlast_error(void);
E void jsonrpc_process(char *buffer, void *userdata);
E int jsonrpc_register_method(const char *name, JSONRPCMethodFunc func);
E int jsonrpc_unregister_method(const char *method);

E char *jsonrpc_array(int argc, ...);
E char *jsonrpc_double(char *buf, double value);
E char *jsonrpc_boolean(char *buf, int value);
E char *jsonrpc_string(char *buf, const char *value);
E char *jsonrpc_integer(char *buf, int value);
E char *jsonrpc_time2date(char *buf, time_t t);

E int jsonrpc_set_options(int type, const char *value);
E void jsonrpc_set_buffer(char *(*func)(char *buffer, int len));
E void jsonrpc_generic_error(int code, const char *string);
E void jsonrpc_send(int argc, ...);
E int jsonrpc_about(void *userdata, int ac, char **av);
E char *jsonrpc_char_encode(char *outbuffer, const char *s1);
E char *jsonrpc_decode_string(char *buf);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
