/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures related to services psuedo-clients.
 *
 * $Id: services.h 7465 2007-01-14 02:50:26Z nenolod $
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

  uint32_t ca_vop;		/* xop access levels */
  uint32_t ca_hop;
  uint32_t ca_aop;
  uint32_t ca_sop;

  char *trigger;		/* trigger, e.g. !, ` or .    */

  boolean_t changets;		/* use TS to better deop people */

  service_t *me;                /* our user_t struct          */
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
};

struct saslsvs_
{
  list_t pending;

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
#define STR_INSUFFICIENT_PARAMS "Insufficient parameters for \2%s\2."
#define STR_INVALID_PARAMS "Invalid parameters for \2%s\2."

/* atheme.c */
E chansvs_t chansvs;
E globsvs_t globsvs;
E opersvs_t opersvs;
E memosvs_t memosvs;
E nicksvs_t nicksvs;
E saslsvs_t saslsvs;
E gamesvs_t gamesvs;

/* servtree.c */
E service_t *fcmd_agent;

/* services.c */
E int authservice_loaded;
E int use_myuser_access;
E int use_svsignore;

E int ban(user_t *source, channel_t *chan, user_t *target);
E int remove_ban_exceptions(user_t *source, channel_t *chan, user_t *target);
E void join(char *chan, char *nick);
E void joinall(char *name);
E void partall(char *name);
E void verbose(mychan_t *mychan, char *fmt, ...);
E void snoop(char *fmt, ...);
E void notice(char *from, char *to, char *message, ...);
E void command_fail(sourceinfo_t *si, faultcode_t code, const char *fmt, ...);
E void command_success_nodata(sourceinfo_t *si, const char *fmt, ...);
E void command_success_string(sourceinfo_t *si, const char *result, const char *fmt, ...);
E const char *get_source_name(sourceinfo_t *si);
E const char *get_source_mask(sourceinfo_t *si);
E const char *get_oper_name(sourceinfo_t *si);
E void wallops(char *, ...);
E void verbose_wallops(char *, ...);

/* ptasks.c */
E void handle_topic(channel_t *, char *, time_t, char *);
E int floodcheck(user_t *, user_t *);

/* ctcp-common.c */
E void common_ctcp_init(void);
E unsigned int handle_ctcp_common(sourceinfo_t *si, char *, char *);

#endif
