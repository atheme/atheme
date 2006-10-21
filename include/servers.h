/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures related to network servers.
 *
 * $Id: servers.h 6805 2006-10-21 19:37:11Z jilles $
 */

#ifndef SERVERS_H
#define SERVERS_H

typedef struct tld_ tld_t;

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

	server_t *uplink; /* uplink server */
	list_t children;  /* children linked to me */
	list_t userlist;  /* users attached to me */
};

#define SF_HIDE        0x00000001
#define SF_EOB         0x00000002 /* Burst finished (we have all users/channels) -- jilles */
#define SF_EOB2        0x00000004 /* Is EOB but an uplink is not (for P10) */

/* tld list struct */
struct tld_ {
  char *name;
};

#define SERVER_NAME(serv)	((serv)->sid[0] ? (serv)->sid : (serv)->name)
#define ME			(ircd->uses_uid ? me.numeric : me.name)

/* node.c */
E dictionary_tree_t *servlist;
E list_t tldlist;

E tld_t *tld_add(const char *name);
E void tld_delete(const char *name);
E tld_t *tld_find(const char *name);

E server_t *server_add(const char *name, uint8_t hops, const char *uplink, const char *id, const char *desc);
E void server_delete(const char *name);
E server_t *server_find(const char *name);

#endif
