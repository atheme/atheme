/*
 * Copyright (c) 2005 atheme.org
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Initialization functions.
 *
 * $Id: libatheme.c 2677 2005-10-06 04:22:32Z nenolod $
 */

#include "atheme.h"

static void generic_claro_log(uint32_t, const char *format, ...);
void (*clog)(uint32_t, const char *format, ...) = generic_claro_log;

static void generic_claro_log(uint32_t level, const char *format, ...)
{
	char buf[BUFSIZE];
	va_list args;

	va_start(args, format);
	vsnprintf(buf, BUFSIZE, format, args);
	va_end(args);

	puts(buf);
}

void libclaro_init(void (*ilog)(uint32_t, const char *format, ...))
{
	if (ilog)
		clog = ilog;

	clog(LG_DEBUG, "claro: starting up base code...");

	event_init();
	initBlockHeap();
	init_dlink_nodes();
	hooks_init();
	callback_init();
	init_netio();
	init_socket_queues();

	clog(LG_DEBUG, "claro: .. done");
}
	
