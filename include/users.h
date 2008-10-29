/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for connected clients.
 *
 * $Id: users.h 8235 2007-05-06 22:54:53Z jilles $
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
	char uid[IDLEN]; /* Used for TS6, P10, IRCNet ircd. */
	char ip[HOSTIPLEN];

	list_t channels;

	server_t *server;
	myuser_t *myuser;

	unsigned int offenses;
	unsigned int msgs; /* times FLOOD_MSGS_FACTOR */
	time_t lastmsg;

	unsigned int flags;

	time_t ts;
};

#define FLOOD_MSGS_FACTOR 256

#define UF_AWAY        0x00000002
#define UF_INVIS       0x00000004
#define UF_IRCOP       0x00000010
#define UF_ADMIN       0x00000020
#define UF_SEENINFO    0x00000080
#define UF_IMMUNE      0x00000100 /* user is immune from kickban, don't bother enforcing akicks */
#define UF_HIDEHOSTREQ 0x00000200 /* host hiding requested */
#define UF_SOPER_PASS  0x00000400 /* services oper pass entered */
#define UF_DOENFORCE   0x00000800 /* introduce enforcer when nick changes */
#define UF_ENFORCER    0x00001000 /* this is an enforcer client */
#define UF_WASENFORCED 0x00002000 /* this user was FNCed once already */

#define CLIENT_NAME(user)	((user)->uid[0] ? (user)->uid : (user)->nick)

/* function.c */
E boolean_t is_ircop(user_t *user);
E boolean_t is_admin(user_t *user);
E boolean_t is_internal_client(user_t *user);

/* users.c */
E mowgli_patricia_t *userlist;
E mowgli_patricia_t *uidlist;

E void init_users(void);

E user_t *user_add(const char *nick, const char *user, const char *host, const char *vhost, const char *ip, const char *uid, const char *gecos, server_t *server, time_t ts);
E void user_delete(user_t *u);
E user_t *user_find(const char *nick);
E user_t *user_find_named(const char *nick);
E void user_changeuid(user_t *u, const char *uid);
E boolean_t user_changenick(user_t *u, const char *nick, time_t ts);
E void user_mode(user_t *user, const char *modes);

/* uid.c */
E void init_uid(void);
E const char *uid_get(void);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
