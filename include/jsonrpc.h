/*
 * Copyright (c) 2005 Atheme Development Group
 *
 * JSONRPC library header
 *
 * $Id: jsonrpc.h 7779 2007-03-03 13:55:42Z pippijn $
 */
#ifndef JSONRPC_H
#define JSONRPC_H

#include "atheme.h"

#define stricmp strcasecmp

#define JSONRPC_ERR_OK			0
#define JSONRPC_ERR_MEMORY		1
#define JSONRPC_ERR_PARAMS		2
#define JSONRPC_ERR_EXISTS		3
#define JSONRPC_ERR_NOEXIST		4
#define JSONRPC_ERR_NOUSER		5
#define JSONRPC_ERR_NOLOAD		6
#define JSONRPC_ERR_NOUNLOAD		7
#define JSONRPC_ERR_SYNTAX		8
#define JSONRPC_ERR_NODELETE		9
#define JSONRPC_ERR_UNKNOWN		10
#define JSONRPC_ERR_FILE_IO		11
#define JSONRPC_ERR_NOSERVICE		12
#define JSONRPC_ERR_NO_MOD_NAME		13

#define JSONRPC_BUFSIZE			1024

/*
 * Header that defines much of the json/jsonrpc library
 */

E void jsonrpc_generic_error(json_object_t *, faultcode_t, char *);
E void jsonrpc_parse_error(json_object_t *, faultcode_t, char *);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
