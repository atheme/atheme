/*
 * Copyright (c) 2005 atheme.org
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Initialization functions.
 *
 * $Id: claro.c 7273 2006-11-25 00:25:20Z jilles $
 */

#include <org.atheme.claro.base>
#include "claro_internal.h"

DECLARE_CLARO_ASSEMBLY_V1
(
	"org.atheme.claro.base",
	"Atheme Development Group <http://www.libclaro.org>",
	"$Id: claro.c 7273 2006-11-25 00:25:20Z jilles $",
	"20051020"
);

int runflags;
claro_state_t claro_state;

static void generic_claro_log(uint32_t, const char *format, ...);
void (*claro_log)(uint32_t, const char *format, ...) = generic_claro_log;

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
		claro_log = ilog;

	claro_log(LG_DEBUG, "claro: starting up base code...");

	event_init();
	initBlockHeap();
	init_dlink_nodes();
	hooks_init();
	init_netio();
	init_socket_queues();

	claro_log(LG_DEBUG, "claro: .. done");
}
	
