/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures related to network servers.
 *
 * $Id: servers.h 1137 2005-07-25 08:28:51Z nenolod $
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

	server_t *uplink;
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
	char *numeric;

	node_t	*node;

	uint32_t port;

	connection_t *conn;
};

extern uplink_t *uplink_add(char *name, char *host, char *password, char *vhost, char *numeric, int port);
extern void uplink_delete(uplink_t *u);
extern uplink_t *uplink_find(char *name);
extern list_t uplinks;
extern uplink_t *curr_uplink;
extern void uplink_connect(void);

#endif
