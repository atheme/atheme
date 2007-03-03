/*
 * Copyright (C) 2005 Jilles Tjoelker, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Fine grained services operator privileges
 *
 * $Id: privs.h 7771 2007-03-03 12:46:36Z pippijn $
 */

#ifndef PRIVS_H
#define PRIVS_H

#define PRIV_NONE            NULL

/* nickserv/userserv */
#define PRIV_USER_AUSPEX     "user:auspex"
#define PRIV_USER_ADMIN      "user:admin"
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
/* generic */
#define PRIV_SERVER_AUSPEX   "general:auspex"
#define PRIV_VIEWPRIVS       "general:viewprivs"
#define PRIV_FLOOD           "general:flood"
#define PRIV_METADATA        "general:metadata"
#define PRIV_ADMIN           "general:admin"
/* operserv */
#define PRIV_OMODE           "operserv:omode"
#define PRIV_AKILL           "operserv:akill"
#define PRIV_MASS_AKILL      "operserv:massakill"
#define PRIV_JUPE            "operserv:jupe"
#define PRIV_NOOP            "operserv:noop"
#define PRIV_GLOBAL          "operserv:global"
#define PRIV_GRANT           "operserv:grant"

/* obsolete access levels */
#define AC_NONE NULL
/* please do not use the following anymore */
#define AC_IRCOP "special:ircop"
#define AC_SRA "general:admin"

struct operclass_ {
  char *name;
  char *privs; /* priv1 priv2 priv3... */
  int flags;
};

#define OPERCLASS_NEEDOPER	0x1 /* only give privs to IRCops */

/* soper list struct */
struct soper_ {
  myuser_t *myuser;
  char *name;
  operclass_t *operclass;
  char *classname;
  int flags;
};

#define SOPER_CONF	0x1 /* oper is listed in atheme.conf */

/* privs.c */
E list_t operclasslist;
E list_t soperlist;

E void init_privs(void);

E operclass_t *operclass_add(char *name, char *privs);
E void operclass_delete(operclass_t *operclass);
E operclass_t *operclass_find(char *name);

E soper_t *soper_add(char *name, char *classname, int flags);
E void soper_delete(soper_t *soper);
E soper_t *soper_find(myuser_t *myuser);
E soper_t *soper_find_named(char *name);

E boolean_t is_soper(myuser_t *myuser);
E boolean_t is_conf_soper(myuser_t *myuser);

/* has_any_privs(): used to determine whether we should give detailed
 * messages about disallowed things
 * warning: do not use this for any kind of real privilege! */
E boolean_t has_any_privs(sourceinfo_t *);
E boolean_t has_any_privs_user(user_t *);
/* has_priv(): for sources of commands */
E boolean_t has_priv(sourceinfo_t *, const char *);
/* has_priv_user(): for online users */
E boolean_t has_priv_user(user_t *, const char *);
/* has_priv_myuser(): channel succession etc */
E boolean_t has_priv_myuser(myuser_t *, const char *);
/* has_priv_operclass(): /os specs etc */
E boolean_t has_priv_operclass(operclass_t *, const char *);
/* has_all_operclass(): checks if source has all privs in operclass */
E boolean_t has_all_operclass(sourceinfo_t *, operclass_t *);

#endif /* PRIVS_H */

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
