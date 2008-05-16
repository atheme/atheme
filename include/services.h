/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures related to services psuedo-clients.
 *
 * $Id: services.h 8417 2007-06-08 00:48:04Z nenolod $
 */

#ifndef SERVICES_H
#define SERVICES_H

typedef struct chansvs_ chansvs_t;
typedef struct globsvs_ globsvs_t;
typedef struct opersvs_ opersvs_t;
typedef struct memosvs_ memosvs_t;
typedef struct nicksvs_ nicksvs_t;
typedef struct saslsvs_ saslsvs_t;
typedef struct gamesvs_ gamesvs_t;

/* core services */
struct chansvs_
{
  char *nick;                   /* the IRC client's nickname  */
  char *user;                   /* the IRC client's username  */
  char *host;                   /* the IRC client's hostname  */
  char *real;                   /* the IRC client's realname  */
  char *disp;			/* the IRC client's dispname  */

  boolean_t fantasy;		/* enable fantasy commands    */

  unsigned int ca_vop;		/* xop access levels */
  unsigned int ca_hop;
  unsigned int ca_aop;
  unsigned int ca_sop;

  char *trigger;		/* trigger, e.g. !, ` or .    */

  boolean_t changets;		/* use TS to better deop people */

  service_t *me;                /* our user_t struct          */

  unsigned int expiry;		/* expiry time                */

  unsigned int maxchanacs;	/* max entries in chanacs list */
  unsigned int maxfounders;	/* max founders per channel    */

  char *deftemplates;		/* default templates          */
};

struct globsvs_
{
  char *nick;
  char *user;
  char *host;
  char *real;
  char *disp;			/* the IRC client's dispname  */
   
  service_t *me;
};

struct opersvs_
{
  char *nick;
  char *user;
  char *host;
  char *real;
  char *disp;			/* the IRC client's dispname  */
   
  service_t *me;
};

struct memosvs_
{
  char   *nick;
  char   *user;
  char   *host;
  char   *real;
  char   *disp;

  service_t *me;
};

/* authentication services */
struct nicksvs_
{
  boolean_t  spam;
  boolean_t  no_nick_ownership;

  char   *nick;
  char   *user;
  char   *host;
  char   *real;
  char   *disp;			/* the IRC client's dispname  */

  service_t *me;

  unsigned int expiry;          /* expiry time                */
  unsigned int enforce_expiry;  /* expiry time                */
  unsigned int enforce_delay;   /* delay for nickname enforce */
};

struct saslsvs_
{
  char   *nick;
  char   *user;
  char   *host;
  char   *real;
  char   *disp;			/* the IRC client's dispname  */

  service_t *me;
};

struct gamesvs_
{
  char   *nick;
  char   *user;
  char   *host;
  char   *real;
  char   *disp;			/* the IRC client's dispname  */

  service_t *me;
};

/* help us keep consistent messages */
#define STR_INSUFFICIENT_PARAMS _("Insufficient parameters for \2%s\2.")
#define STR_INVALID_PARAMS _("Invalid parameters for \2%s\2.")

/* atheme.c */
E chansvs_t chansvs;
E globsvs_t globsvs;
E opersvs_t opersvs;
E memosvs_t memosvs;
E nicksvs_t nicksvs;
E saslsvs_t saslsvs;
E gamesvs_t gamesvs;

/* services.c */
E int authservice_loaded;
E int use_myuser_access;
E int use_svsignore;
E int use_privmsg;
E int use_account_private;
E int use_channel_private;

E int ban(user_t *source, channel_t *chan, user_t *target);
E int remove_banlike(user_t *source, channel_t *chan, int type, user_t *target);
E int remove_ban_exceptions(user_t *source, channel_t *chan, user_t *target);
E void kill_user(user_t *source, user_t *victim, const char *fmt, ...) PRINTFLIKE(3, 4);
E void introduce_enforcer(const char *nick);
E void join(char *chan, char *nick);
E void joinall(char *name);
E void part(char *chan, char *nick);
E void partall(char *name);
E void verbose(mychan_t *mychan, const char *fmt, ...) PRINTFLIKE(2, 3);
E void snoop(const char *fmt, ...) PRINTFLIKE(1, 2);
E void notice(const char *from, const char *to, const char *message, ...) PRINTFLIKE(3, 4);
E void command_fail(sourceinfo_t *si, faultcode_t code, const char *fmt, ...) PRINTFLIKE(3, 4);
E void command_success_nodata(sourceinfo_t *si, const char *fmt, ...) PRINTFLIKE(2, 3);
E void command_success_string(sourceinfo_t *si, const char *result, const char *fmt, ...) PRINTFLIKE(3, 4);
E void command_success_table(sourceinfo_t *si, table_t *table);
E const char *get_source_name(sourceinfo_t *si);
E const char *get_source_mask(sourceinfo_t *si);
E const char *get_oper_name(sourceinfo_t *si);
E const char *get_storage_oper_name(sourceinfo_t *si);
E void wallops(const char *, ...) PRINTFLIKE(1, 2);
E void verbose_wallops(const char *, ...) PRINTFLIKE(1, 2);

/* ptasks.c */
E void handle_topic(channel_t *, char *, time_t, char *);
E int floodcheck(user_t *, user_t *);

/* ctcp-common.c */
E void common_ctcp_init(void);
E unsigned int handle_ctcp_common(sourceinfo_t *si, char *, char *);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
