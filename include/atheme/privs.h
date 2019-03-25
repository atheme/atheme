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

#define PRIV_NONE            NULL

/* nickserv/userserv */
#define PRIV_USER_AUSPEX     "user:auspex"
#define PRIV_USER_ADMIN      "user:admin"
#define PRIV_USER_SENDPASS   "user:sendpass"
#define PRIV_USER_VHOST      "user:vhost"
#define PRIV_USER_FREGISTER  "user:fregister"
/* chanserv */
#define PRIV_CHAN_AUSPEX     "chan:auspex"
#define PRIV_CHAN_ADMIN      "chan:admin"
#define PRIV_CHAN_CMODES     "chan:cmodes"
#define PRIV_JOIN_STAFFONLY  "chan:joinstaffonly"
/* nickserv/userserv+chanserv */
#define PRIV_MARK            "user:mark"
#define PRIV_HOLD            "user:hold"
#define PRIV_REG_NOLIMIT     "user:regnolimit"
/* groupserv */
#define PRIV_GROUP_ADMIN     "group:admin"
#define PRIV_GROUP_AUSPEX    "group:auspex"
/* generic */
#define PRIV_SERVER_AUSPEX   "general:auspex"
#define PRIV_VIEWPRIVS       "general:viewprivs"
#define PRIV_FLOOD           "general:flood"
#define PRIV_HELPER	     "general:helper"
#define PRIV_METADATA        "general:metadata"
#define PRIV_ADMIN           "general:admin"
/* operserv */
#define PRIV_OMODE           "operserv:omode"
#define PRIV_AKILL           "operserv:akill"
#define PRIV_MASS_AKILL      "operserv:massakill"
#define PRIV_AKILL_ANYMASK   "operserv:akill-anymask"
#define PRIV_JUPE            "operserv:jupe"
#define PRIV_NOOP            "operserv:noop"
#define PRIV_GLOBAL          "operserv:global"
#define PRIV_GRANT           "operserv:grant"
/* saslserv */
#define PRIV_IMPERSONATE_CLASS_FMT	"impersonate:class:%s"
#define PRIV_IMPERSONATE_ENTITY_FMT	"impersonate:entity:%s"
#define PRIV_IMPERSONATE_ANY		"impersonate:any"

/* other access levels */
#define AC_NONE NULL /* anyone */
#define AC_DISABLED "special:disabled" /* noone */
#define AC_AUTHENTICATED "special:authenticated"
/* please do not use the following anymore */
#define AC_IRCOP "special:ircop"
#define AC_SRA "general:admin"

struct operclass
{
	char *          name;
	char *          privs;  // priv1 priv2 priv3...
	int             flags;
	mowgli_node_t   node;
};

#define OPERCLASS_NEEDOPER	0x1U /* only give privs to IRCops */
#define OPERCLASS_BUILTIN	0x2U /* builtin */

/* soper list struct */
struct soper
{
	struct myuser *         myuser;
	char *                  name;
	struct operclass *      operclass;
	char *                  classname;
	int                     flags;
	char *                  password;
};

#define SOPER_CONF	0x1U /* oper is listed in atheme.conf */

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
