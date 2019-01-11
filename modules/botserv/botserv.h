/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2009 Celestin, et al.
 *
 * BotServ common definitions.
 */

#include "sysconf.h"

#ifndef ATHEME_MOD_BOTSERV_BOTSERV_H
#define ATHEME_MOD_BOTSERV_BOTSERV_H 1

#include "atheme.h"

struct botserv_bot
{
	struct service *me;
	char *nick;
	char *user;
	char *host;
	char *real;
	mowgli_node_t bnode;
	bool private;
	time_t registered;
};

typedef struct botserv_bot *(fn_botserv_bot_find)(char *name);

#endif /* !ATHEME_MOD_BOTSERV_BOTSERV_H */
