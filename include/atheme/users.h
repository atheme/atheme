/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * Data structures for connected clients.
 */

#ifndef ATHEME_INC_USERS_H
#define ATHEME_INC_USERS_H 1

#include <atheme/common.h>
#include <atheme/object.h>
#include <atheme/stdheaders.h>
#include <atheme/structures.h>

struct user
{
	struct atheme_object    parent;
	stringref               nick;
	stringref               user;
	stringref               host;           // Real host
	stringref               gecos;
	stringref               chost;          // Cloaked host
	stringref               vhost;          // Visible host
	stringref               uid;            // Used for TS6, P10, IRCNet ircd
	stringref               ip;
	mowgli_list_t           channels;
	struct server *         server;
	struct myuser *         myuser;
	unsigned int            offenses;
	unsigned int            msgs;           // times FLOOD_MSGS_FACTOR
	time_t                  lastmsg;
	unsigned int            flags;
	time_t                  ts;
	mowgli_node_t           snode;          // for struct server -> userlist
	char *                  certfp;         // client certificate fingerprint
};

#define UF_AWAY        0x00000002U
#define UF_INVIS       0x00000004U
#define UF_DOING_SASL  0x00000008U
#define UF_IRCOP       0x00000010U
#define UF_ADMIN       0x00000020U
#define UF_SEENINFO    0x00000080U
#define UF_IMMUNE      0x00000100U /* user is immune from kickban, don't bother enforcing akicks */
#define UF_HIDEHOSTREQ 0x00000200U /* host hiding requested */
#define UF_SOPER_PASS  0x00000400U /* services oper pass entered */
#define UF_DOENFORCE   0x00000800U /* introduce enforcer when nick changes */
#define UF_ENFORCER    0x00001000U /* this is an enforcer client */
#define UF_WASENFORCED 0x00002000U /* this user was FNCed once already */
#define UF_DEAF        0x00004000U /* user does not receive channel msgs */
#define UF_SERVICE     0x00008000U /* user is a service (e.g. +S on charybdis) */
#define UF_KLINESENT   0x00010000U /* we've sent a kline for this user */
#define UF_CUSTOM1     0x00020000U /* for internal use by protocol modules */
#define UF_CUSTOM2     0x00040000U
#define UF_CUSTOM3     0x00080000U
#define UF_CUSTOM4     0x00100000U

#define CLIENT_NAME(user)	((user)->uid != NULL ? (user)->uid : (user)->nick)

/* function.c */
bool is_internal_client(struct user *user);
bool is_autokline_exempt(struct user *user);

/* users.c */
extern mowgli_patricia_t *userlist;
extern mowgli_patricia_t *uidlist;

void init_users(void);

struct user *user_add(const char *nick, const char *user, const char *host, const char *vhost, const char *ip, const char *uid, const char *gecos, struct server *server, time_t ts);
void user_delete(struct user *u, const char *comment);
struct user *user_find(const char *nick);
struct user *user_find_named(const char *nick);
void user_changeuid(struct user *u, const char *uid);
bool user_changenick(struct user *u, const char *nick, time_t ts);
void user_mode(struct user *user, const char *modes);
void user_sethost(struct user *source, struct user *target, const char *host);
const char *user_get_umodestr(struct user *u);
struct chanuser *find_user_banned_channel(struct user *u, char ban_type);

/* uid.c */
void init_uid(void);
const char *uid_get(void);

#endif /* !ATHEME_INC_USERS_H */
