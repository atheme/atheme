/*
 * Copyright (c) 2005 atheme.org
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Initialization functions.
 *
 * $Id: claro.c 3325 2005-10-31 03:27:49Z nenolod $
 */

#include <org.atheme.claro.base>

DECLARE_CLARO_ASSEMBLY_V1
(
	"org.atheme.claro.base",
	"Atheme Development Group <http://www.libclaro.org>",
	"$Id: claro.c 3325 2005-10-31 03:27:49Z nenolod $",
	"20051020"
);

int runflags;
claro_state_t claro_state;

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
	init_netio();
	init_socket_queues();

	clog(LG_DEBUG, "claro: .. done");
}
	
