/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for connected clients.
 *
 * $Id: users.h 5644 2006-07-02 01:32:36Z jilles $
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

#endif
