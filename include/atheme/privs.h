/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Jilles Tjoelker, et al.
 *
 * Fine grained services operator privileges
 */

#ifndef ATHEME_INC_PRIVS_H
#define ATHEME_INC_PRIVS_H 1

#include <atheme/sourceinfo.h>
#include <atheme/stdheaders.h>

// NickServ/UserServ
#define PRIV_USER_ADMIN                 "user:admin"
#define PRIV_USER_AUSPEX                "user:auspex"
#define PRIV_USER_FREGISTER             "user:fregister"
#define PRIV_USER_SENDPASS              "user:sendpass"
#define PRIV_USER_VHOST                 "user:vhost"

// ChanServ
#define PRIV_CHAN_ADMIN                 "chan:admin"
#define PRIV_CHAN_AUSPEX                "chan:auspex"
#define PRIV_CHAN_CMODES                "chan:cmodes"
#define PRIV_JOIN_STAFFONLY             "chan:joinstaffonly"

// NickServ/UserServ & ChanServ
#define PRIV_HOLD                       "user:hold"
#define PRIV_LOGIN_NOLIMIT              "user:loginnolimit"
#define PRIV_MARK                       "user:mark"
#define PRIV_REG_NOLIMIT                "user:regnolimit"

// GroupServ
#define PRIV_GROUP_ADMIN                "group:admin"
#define PRIV_GROUP_AUSPEX               "group:auspex"

// OperServ
#define PRIV_AKILL                      "operserv:akill"
#define PRIV_AKILL_ANYMASK              "operserv:akill-anymask"
#define PRIV_GLOBAL                     "operserv:global"
#define PRIV_GRANT                      "operserv:grant"
#define PRIV_JUPE                       "operserv:jupe"
#define PRIV_MASS_AKILL                 "operserv:massakill"
#define PRIV_NOOP                       "operserv:noop"
#define PRIV_OMODE                      "operserv:omode"

// SASLServ
#define PRIV_IMPERSONATE_ANY            "impersonate:any"
#define PRIV_IMPERSONATE_CLASS_FMT      "impersonate:class:%s"
#define PRIV_IMPERSONATE_ENTITY_FMT     "impersonate:entity:%s"

// Generic
#define PRIV_ADMIN                      "general:admin"
#define PRIV_SERVER_AUSPEX              "general:auspex"
#define PRIV_FLOOD                      "general:flood"
#define PRIV_HELPER                     "general:helper"
#define PRIV_METADATA                   "general:metadata"
#define PRIV_VIEWPRIVS                  "general:viewprivs"

// Other privileges
#define AC_AUTHENTICATED                "special:authenticated"
#define AC_DISABLED                     "special:disabled"

// This is used to denote that no privilege is required for a command
#define AC_NONE                         NULL

// These are only retained for compatibility. Do not use.
#define PRIV_NONE                       NULL
#define AC_SRA                          "general:admin"
#define AC_IRCOP                        "special:ircop"

// Flags for (struct operclass).flags
#define OPERCLASS_NEEDOPER              0x1U // Only give privs to IRCOps
#define OPERCLASS_BUILTIN               0x2U // Regardless of atheme.conf

// Flags for (struct soper).flags
#define SOPER_CONF                      0x1U // Oper is listed in atheme.conf

struct operclass
{
	char *              name;
	char *              privs;      // Space-separated list of privileges
	int                 flags;
	mowgli_node_t       node;
};

struct soper
{
	struct myuser *     myuser;
	char *              name;
	struct operclass *  operclass;
	char *              classname;
	int                 flags;
	char *              password;
};

/* privs.c */
extern mowgli_list_t operclasslist;
extern mowgli_list_t soperlist;

void init_privs(void);

struct operclass *operclass_add(const char *name, const char *privs, int flags);
void operclass_delete(struct operclass *operclass);
struct operclass *operclass_find(const char *name);

struct soper *soper_add(const char *name, const char *classname, int flags, const char *password);
void soper_delete(struct soper *soper);
struct soper *soper_find(struct myuser *myuser);
struct soper *soper_find_named(const char *name);

bool is_soper(struct myuser *myuser);
bool is_conf_soper(struct myuser *myuser);

/* has_any_privs(): used to determine whether we should give detailed
 * messages about disallowed things
 * warning: do not use this for any kind of real privilege! */
bool has_any_privs(struct sourceinfo *);
bool has_any_privs_user(struct user *);
/* has_priv(): for sources of commands */
bool has_priv(struct sourceinfo *, const char *);
/* has_priv_user(): for online users */
bool has_priv_user(struct user *, const char *);
/* has_priv_myuser(): channel succession etc */
bool has_priv_myuser(struct myuser *, const char *);
/* has_priv_operclass(): /os specs etc */
bool has_priv_operclass(struct operclass *, const char *);
/* has_all_operclass(): checks if source has all privs in operclass */
bool has_all_operclass(struct sourceinfo *, struct operclass *);

/* get_sourceinfo_soper(): get the specific operclass role which is granting
 * privilege authority
 */
const struct soper *get_sourceinfo_soper(struct sourceinfo *si);
/* get_sourceinfo_operclass(): get the specific operclass role which is granting
 * privilege authority
 */
const struct operclass *get_sourceinfo_operclass(struct sourceinfo *si);

#endif /* !ATHEME_INC_PRIVS_H */
