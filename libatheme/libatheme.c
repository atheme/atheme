/*
 * $Id: libatheme.c 2235 2005-09-14 07:29:13Z nenolod $
 */

#include "atheme.h"

void (*libatheme_log)(int, char *format, ...) = NULL;

void libatheme_init(void (*ilog)(int, char *format, ...))
{
	if (ilog)
		libatheme_log = ilog;

	slog(LG_DEBUG, "libatheme_init(): starting up...");

	event_init();
	init_balloc();
	init_dlink_nodes();
	hooks_init();
	callback_init();
	init_netio();
	init_socket_queues();

	slog(LG_DEBUG, "libatheme_init(): ready to go.");
}
	
