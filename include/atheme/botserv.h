/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2009 Celestin, et al.
 * Copyright (C) 2020 Atheme Development Group (https://atheme.github.io/)
 *
 * BotServ common definitions.
 */

#ifndef ATHEME_INC_BOTSERV_H
#define ATHEME_INC_BOTSERV_H 1

#include <atheme/stdheaders.h>

struct botserv_bot
{
	mowgli_node_t   bnode;
	struct service *me;
	char *          nick;
	char *          user;
	char *          host;
	char *          real;
	time_t          registered;
	bool            private;
};

struct botserv_main_symbols
{
	struct botserv_bot *  (*bot_find)(const char *);
	mowgli_list_t *         bots;
};

#endif /* !ATHEME_INC_BOTSERV_H */
