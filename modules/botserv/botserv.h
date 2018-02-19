/*
 * Copyright (c) 2009 Celestin, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * BotServ common definitions.
 */

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
