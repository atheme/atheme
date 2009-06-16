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
	node_t bnode;
	int private;
	time_t registered;
} botserv_bot_t; 

