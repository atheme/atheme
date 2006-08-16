/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for connected clients.
 *
 * $Id: users.h 6069 2006-08-16 14:28:24Z jilles $
 */

#ifndef USERS_H
#define USERS_H

struct user_
{
	char nick[NICKLEN];
	char user[USERLEN];
	char host[HOSTLEN]; /* Real host */
	char gecos[GECOSLEN];
	char vhost[HOSTLEN]; /* Visible host */
	char uid[NICKLEN]; /* Used for TS6, P10, IRCNet ircd. */
	char ip[HOSTLEN];

	list_t channels;

	server_t *server;
	myuser_t *myuser;

	uint8_t offenses;
	uint8_t msgs;
	time_t lastmsg;

	uint32_t flags;
	int32_t hash;
	int32_t uhash;

	uint32_t ts;
};

#define UF_ISOPER      0x00000001
#define UF_ISAWAY      0x00000002
#define UF_INVIS       0x00000004
#define UF_LOGGEDIN    0x00000008
#define UF_IRCOP       0x00000010
#define UF_ADMIN       0x00000020
#define UF_SEENINFO    0x00000080
#define UF_NICK_WARNED 0x00000100 /* warned about nickstealing, FNC next time */
#define UF_HIDEHOSTREQ 0x00000200 /* host hiding requested */

/* node.c */
E list_t userlist[HASHSIZE];

E user_t *user_add(const char *nick, const char *user, const char *host, const char *vhost, const char *ip, const char *uid, const char *gecos, server_t *server, uint32_t ts);
E void user_delete(user_t *u);
E user_t *user_find(const char *nick);
E user_t *user_find_named(const char *nick);
E void user_changeuid(user_t *u, const char *uid);

#endif
