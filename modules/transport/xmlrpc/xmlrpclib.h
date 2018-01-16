/*
 * Copyright (c) 2005 Atheme Development Group
 *                and Trystan Scott Lee <trystan@nomadirc.net>
 *
 * XMLRPC library header, hacked up for Atheme.
 *
 */
#ifndef XMLRPC_H
#define XMLRPC_H

#include "atheme.h"

#define stricmp strcasecmp

#define XMLRPC_STOP 1
#define XMLRPC_CONT 0

#define XMLRPC_ERR_OK			0
#define XMLRPC_ERR_MEMORY		1
#define XMLRPC_ERR_PARAMS		2
#define XMLRPC_ERR_EXISTS		3
#define XMLRPC_ERR_NOEXIST		4
#define XMLRPC_ERR_NOUSER		5
#define XMLRPC_ERR_NOLOAD      6
#define XMLRPC_ERR_NOUNLOAD    7
#define XMLRPC_ERR_SYNTAX		8
#define XMLRPC_ERR_NODELETE	9
#define XMLRPC_ERR_UNKNOWN		10
#define XMLRPC_ERR_FILE_IO     11
#define XMLRPC_ERR_NOSERVICE   12
#define XMLRPC_ERR_NO_MOD_NAME 13

/*
 * Header that defines much of the xml/xmlrpc library
 */

#define XMLRPC_BUFSIZE         4096
#define XMLLIB_VERSION		 "1.0.0"
#define XMLLIB_AUTHOR		 "Trystan Scott Lee <trystan@nomadirc.net>"

#define XMLRPCCMD XMLRPCCMD_cmdTable
#define CMD_HASH(x)      (((x)[0]&31)<<5 | ((x)[1]&31))	/* Will gen a hash from a string :) */
#define MAX_CMD_HASH 1024

#define XMLRPC_ON "on"
#define XMLRPC_OFF "off"

#define XMLRPC_I4 "i4"
#define XMLRPC_INT "integer"

#define XMLRPC_HTTP_HEADER 1
#define XMLRPC_ENCODE 2
#define XMLRPC_INTTAG 3

typedef int (*XMLRPCMethodFunc)(void *userdata, int ac, char **av);

extern int xmlrpc_getlast_error(void);
extern void xmlrpc_process(char *buffer, void *userdata);
extern int xmlrpc_register_method(const char *name, XMLRPCMethodFunc func);
extern int xmlrpc_unregister_method(const char *method);

extern char *xmlrpc_array(int argc, ...);
extern char *xmlrpc_double(char *buf, double value);
extern char *xmlrpc_boolean(char *buf, int value);
extern char *xmlrpc_string(char *buf, const char *value);
extern char *xmlrpc_integer(char *buf, int value);
extern char *xmlrpc_time2date(char *buf, time_t t);

extern int xmlrpc_set_options(int type, const char *value);
extern void xmlrpc_set_buffer(char *(*func)(char *buffer, int len));
extern void xmlrpc_generic_error(int code, const char *string);
extern void xmlrpc_send(int argc, ...);
extern void xmlrpc_send_string(const char *value);
extern int xmlrpc_about(void *userdata, int ac, char **av);
extern void xmlrpc_char_encode(char *outbuffer, const char *s1);
extern char *xmlrpc_decode_string(char *buf);
extern char *xmlrpc_normalizeBuffer(const char *buf);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
