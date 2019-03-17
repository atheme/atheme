/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * Generic HTTP server
 */

#ifndef ATHEME_INC_HTTPD_H
#define ATHEME_INC_HTTPD_H 1

#include <atheme/stdheaders.h>
#include <atheme/structures.h>

struct path_handler
{
	const char *    path;
	void          (*handler)(struct connection *, void *);
};

struct httpddata
{
	char            method[64];
	char            filename[256];
	char *          requestbuf;
	char *          replybuf;
	int             length;
	int             lengthdone;
	bool            connection_close;
	bool            correct_content_type;
	bool            expect_100_continue;
	bool            sent_reply;
};

#endif /* !ATHEME_INC_HTTPD_H */
