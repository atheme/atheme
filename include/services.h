/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures related to services psuedo-clients.
 */

#ifndef SERVICES_H
#define SERVICES_H

typedef struct nicksvs_ nicksvs_t;

/* The nick/user/host/real strings in these structs simply point
 * to their counterparts in the struct service, and will probably be removed
 * at some point.
 */
/* core services */
struct chansvs
{
  char *nick;                   /* the IRC client's nickname  */
  char *user;                   /* the IRC client's username  */
  char *host;                   /* the IRC client's hostname  */
  char *real;                   /* the IRC client's realname  */

  bool fantasy;		/* enable fantasy commands    */

  char *trigger;		/* trigger, e.g. !, ` or .    */

  bool changets;		/* use TS to better deop people */

  struct service *me;                /* our user_t struct          */

  unsigned int expiry;		/* expiry time                */

  unsigned int akick_time; /* default akick duration */

  unsigned int maxchans;    /* max channels one can register */
  unsigned int maxchanacs;	/* max entries in chanacs list */
  unsigned int maxfounders;	/* max founders per channel    */

  char *founder_flags;		/* default founder flags for new channels */

  char *deftemplates;		/* default templates          */

  bool hide_xop;		/* hide XOP templates	      */
};

/* authentication services */
struct nicksvs_
{
  bool  spam;
  bool  no_nick_ownership;

  char   *nick;
  char   *user;
  char   *host;
  char   *real;

  struct service *me;

  unsigned int maxnicks;        /* max nicknames one can group */
  unsigned int expiry;          /* expiry time                */
  unsigned int enforce_expiry;  /* expiry time                */
  unsigned int enforce_delay;   /* delay for nickname enforce */
  char         *enforce_prefix; /* prefix for enforcement */
  char	       *cracklib_dict; /* cracklib dictionary path */
  mowgli_list_t emailexempts; /* emails exempt from maxusers checks */
};

/* help us keep consistent messages */
#define STR_INSUFFICIENT_PARAMS _("Insufficient parameters for \2%s\2.")
#define STR_INVALID_PARAMS _("Invalid parameters for \2%s\2.")
#define STR_NO_PRIVILEGE _("You do not have the %s privilege.")

/* for command_add_flood(),
 * note that floodcheck() already does one FLOOD_MSGS_FACTOR
 */
#define FLOOD_HEAVY (3 * FLOOD_MSGS_FACTOR)
#define FLOOD_MODERATE FLOOD_MSGS_FACTOR
#define FLOOD_LIGHT 0

/* atheme.c */
extern struct chansvs chansvs;
extern nicksvs_t nicksvs;

/* services.c */
extern int authservice_loaded;
extern int use_myuser_access;
extern int use_svsignore;
extern int use_privmsg;
extern int use_account_private;
extern int use_channel_private;
extern int use_limitflags;

extern int ban(user_t *source, struct channel *chan, user_t *target);
extern int remove_banlike(user_t *source, struct channel *chan, int type, user_t *target);
extern int remove_ban_exceptions(user_t *source, struct channel *chan, user_t *target);

extern void try_kick_real(user_t *source, struct channel *chan, user_t *target, const char *reason);
extern void (*try_kick)(user_t *source, struct channel *chan, user_t *target, const char *reason);

extern void kill_user(user_t *source, user_t *victim, const char *fmt, ...) ATHEME_FATTR_PRINTF(3, 4);
extern void introduce_enforcer(const char *nick);
extern void join(const char *chan, const char *nick);
extern void joinall(const char *name);
extern void part(const char *chan, const char *nick);
extern void partall(const char *name);
extern void myuser_login(struct service *svs, user_t *u, myuser_t *mu, bool sendaccount);
extern void verbose(mychan_t *mychan, const char *fmt, ...) ATHEME_FATTR_PRINTF(2, 3);
extern void (*notice)(const char *from, const char *target, const char *fmt, ...) ATHEME_FATTR_PRINTF(3, 4);
extern void change_notify(const char *from, user_t *to, const char *message, ...) ATHEME_FATTR_PRINTF(3, 4);
extern bool bad_password(struct sourceinfo *si, myuser_t *mu);

extern struct sourceinfo *sourceinfo_create(void);
extern void command_fail(struct sourceinfo *si, enum cmd_faultcode code, const char *fmt, ...) ATHEME_FATTR_PRINTF(3, 4);
extern void command_success_nodata(struct sourceinfo *si, const char *fmt, ...) ATHEME_FATTR_PRINTF(2, 3);
extern void command_success_string(struct sourceinfo *si, const char *result, const char *fmt, ...) ATHEME_FATTR_PRINTF(3, 4);
extern void command_success_table(struct sourceinfo *si, table_t *table);
extern const char *get_source_name(struct sourceinfo *si);
extern const char *get_source_mask(struct sourceinfo *si);
extern const char *get_oper_name(struct sourceinfo *si);
extern const char *get_storage_oper_name(struct sourceinfo *si);
extern const char *get_source_security_label(struct sourceinfo *si);

extern void wallops(const char *, ...) ATHEME_FATTR_PRINTF(1, 2);
extern void verbose_wallops(const char *, ...) ATHEME_FATTR_PRINTF(1, 2);
extern bool check_vhost_validity(struct sourceinfo *si, const char *host);

/* ptasks.c */
extern void handle_topic(struct channel *, const char *, time_t, const char *);
extern int floodcheck(user_t *, user_t *);
extern void command_add_flood(struct sourceinfo *si, unsigned int amount);

/* ctcp-common.c */
extern void common_ctcp_init(void);
extern unsigned int handle_ctcp_common(struct sourceinfo *si, char *, char *);

#ifdef HAVE_LIBQRENCODE
/* qrcode.c */
extern void command_success_qrcode(struct sourceinfo *si, const char *data);
#endif /* HAVE_LIBQRENCODE */

#endif /* !SERVICES_H */
