/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures related to network servers.
 *
 * $Id: servers.h 3129 2005-10-22 21:49:56Z jilles $
 */

#ifndef SERVERS_H
#define SERVERS_H

typedef struct server_ server_t;
typedef struct uplink_ uplink_t;

/* servers struct */
struct server_
{
	char *name;
	char *desc;
	char *sid;

	uint8_t hops;
	uint32_t users;
	uint32_t invis;
	uint32_t opers;
	uint32_t away;

	time_t connected_since;

	uint32_t flags;
	int32_t hash;
	int32_t shash;

	server_t *uplink; /* uplink server */
	list_t children;  /* children linked to me */
	list_t userlist;  /* users attached to me */
};

#define SF_HIDE        0x00000001
#define SF_EOB         0x00000002 /* End of burst acknowledged, not all
                                   * protocols use this -- jilles */

struct uplink_
{
	char *name;
	char *host;
	char *pass;
	char *vhost;

	node_t	*node;

	uint32_t port;

	connection_t *conn;
};

E uplink_t *uplink_add(char *name, char *host, char *password, char *vhost, int port);
E void uplink_delete(uplink_t *u);
E uplink_t *uplink_find(char *name);
E list_t uplinks;
E uplink_t *curr_uplink;
E void uplink_connect(void);
E void connection_dead(void *vptr);

#endif
