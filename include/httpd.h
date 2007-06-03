/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Generic HTTP server
 *
 * $Id$
 */

#ifndef HTTPD_H
#define HTTPD_H

typedef struct path_handler_ path_handler_t;

struct path_handler_
{
	const char *path;
	void (*handler)(connection_t *, void *);
};

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

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
