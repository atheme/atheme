/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * Data structures related to network servers.
 */

#ifndef ATHEME_INC_SERVERS_H
#define ATHEME_INC_SERVERS_H 1

#include <atheme/stdheaders.h>

/* servers struct */
struct server
{
	char *          name;
	char *          desc;
	char *          sid;
	unsigned int    hops;
	unsigned int    users;
	unsigned int    invis;
	unsigned int    opers;
	unsigned int    away;
	time_t          connected_since;
	unsigned int    flags;
	struct server * uplink;         // uplink server
	mowgli_list_t   children;       // children linked to me
	mowgli_list_t   userlist;       // users attached to me
};

#define SF_HIDE        0x00000001U
#define SF_EOB         0x00000002U /* Burst finished (we have all users/channels) -- jilles */
#define SF_EOB2        0x00000004U /* Is EOB but an uplink is not (for P10) */
#define SF_JUPE_PENDING 0x00000008U /* Sent SQUIT request, will introduce jupe when it dies (unconnect semantics) */
#define SF_MASKED      0x00000010U /* Is masked, has no own name (for ircnet) */

/* tld list struct */
struct tld
{
	char *  name;
};

#define SERVER_NAME(serv)	(((serv)->sid && ircd->uses_uid) ? (serv)->sid : (serv)->name)
#define ME			(ircd->uses_uid ? me.numeric : me.name)

/* servers.c */
extern mowgli_patricia_t *servlist;
extern mowgli_list_t tldlist;

void init_servers(void);

struct tld *tld_add(const char *name);
void tld_delete(const char *name);
struct tld *tld_find(const char *name);

struct server *server_add(const char *name, unsigned int hops, struct server *uplink, const char *id, const char *desc);
void server_delete(const char *name);
struct server *server_find(const char *name);

#endif /* !ATHEME_INC_SERVERS_H */
