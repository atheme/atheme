/*
 * Copyright (c) 2009 Celestin, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * BotServ common definitions.
 */

typedef struct {
	service_t *me;
	char *nick;
	char *user;
	char *host;
	char *real;
	mowgli_node_t bnode;
	bool private;
	time_t registered;
} botserv_bot_t; 

typedef botserv_bot_t *fn_botserv_bot_find(char *name);
