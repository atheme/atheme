/*
 * Copyright (c) 2005-2009 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for account information.
 *
 */

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "entity.h"

typedef struct mycertfp_ mycertfp_t;
typedef struct myuser_name_ myuser_name_t;
typedef struct chanacs_ chanacs_t;
typedef struct kline_ kline_t;
typedef struct xline_ xline_t;
typedef struct qline_ qline_t;
typedef struct mymemo_ mymemo_t;
typedef struct svsignore_ svsignore_t;

/* kline list struct */
struct kline_ {
  char *user;
  char *host;
  char *reason;
  char *setby;

  unsigned long number;
  long duration;
  time_t settime;
  time_t expires;
};

/* xline list struct */
struct xline_ {
  char *realname;
  char *reason;
  char *setby;

  unsigned int number;
  long duration;
  time_t settime;
  time_t expires;
};

/* qline list struct */
struct qline_ {
  char *mask;
  char *reason;
  char *setby;

  unsigned int number;
  long duration;
  time_t settime;
  time_t expires;
};

/* services ignore struct */
struct svsignore_ {
  svsignore_t *svsignore;

  char *mask;
  time_t settime;
  char *setby;
  char *reason;
};

/* services accounts */
struct myuser_
{
  myentity_t ent;
  char pass[PASSLEN];
  char *email;

  mowgli_list_t logins; /* user_t's currently logged in to this */
  time_t registered;
  time_t lastlogin;

  soper_t *soper;

  unsigned int flags;
  
  mowgli_list_t memos; /* store memos */
  unsigned short memoct_new;
  unsigned short memo_ratelimit_num; /* memos sent recently */
  time_t memo_ratelimit_time; /* last time a memo was sent */
  mowgli_list_t memo_ignores;

  /* openservices patch */
  mowgli_list_t access_list;

  mowgli_list_t nicks; /* registered nicks, must include mu->name if nonempty */

  mowgli_list_t subscriptions; /* presence subscriptors */
  
  language_t *language;

  mowgli_list_t cert_fingerprints;
};

#define MU_HOLD        0x00000001
#define MU_NEVEROP     0x00000002
#define MU_NOOP        0x00000004
#define MU_WAITAUTH    0x00000008
#define MU_HIDEMAIL    0x00000010
#define MU_NOMEMO      0x00000040
#define MU_EMAILMEMOS  0x00000080
#define MU_CRYPTPASS   0x00000100
#define MU_NOBURSTLOGIN 0x00000400
#define MU_ENFORCE     0x00000800 /* XXX limited use at this time */
#define MU_USE_PRIVMSG 0x00001000 /* use PRIVMSG */
#define MU_PRIVATE     0x00002000
#define MU_QUIETCHG    0x00004000
#define MU_NOGREET     0x00008000
#define MU_REGNOLIMIT  0x00010000

/* memoserv rate limiting parameters */
#define MEMO_MAX_NUM   5
#define MEMO_MAX_TIME  180

/* registered nick */
struct mynick_
{
  object_t parent;

  char nick[NICKLEN];

  myuser_t *owner;

  time_t registered;
  time_t lastseen;

  mowgli_node_t node; /* for myuser_t.nicks */
};

/* record about a name that used to exist */
struct myuser_name_
{
  object_t parent;

  char name[NICKLEN];
};

struct mycertfp_
{
  myuser_t *mu;

  char *certfp;
  
  mowgli_node_t node;
};

struct mychan_
{
  object_t parent;

  char *name;

  channel_t *chan;
  mowgli_list_t chanacs;
  time_t registered;
  time_t used;

  unsigned int mlock_on;
  unsigned int mlock_off;
  unsigned int mlock_limit;
  char *mlock_key;

  unsigned int flags;
};

#define MC_HOLD        0x00000001
#define MC_NOOP        0x00000002
#define MC_LIMITFLAGS  0x00000004
#define MC_SECURE      0x00000008
#define MC_VERBOSE     0x00000010
#define MC_RESTRICTED  0x00000020
#define MC_KEEPTOPIC   0x00000040
#define MC_VERBOSE_OPS 0x00000080
#define MC_TOPICLOCK   0x00000100
#define MC_GUARD       0x00000200
#define MC_PRIVATE     0x00000400

/* The following are temporary state */
#define MC_INHABIT     0x80000000 /* we're on channel to enforce akick/staffonly/close */
#define MC_MLOCK_CHECK 0x40000000 /* we need to check mode locks */
#define MC_FORCEVERBOSE 0x20000000 /* fantasy cmd in progress, be verbose */
#define MC_RECREATED   0x10000000 /* created with new channelTS */

#define MC_VERBOSE_MASK (MC_VERBOSE | MC_VERBOSE_OPS)

/* struct for channel access list */
struct chanacs_
{
	object_t parent;

	myentity_t *entity;
	mychan_t *mychan;
	char     *host;
	unsigned int  level;
	time_t    tmodified;
	mowgli_node_t    cnode;
};

/* the new atheme-style channel flags */
#define CA_VOICE         0x00000001 /* Ability to use voice/devoice command. */
#define CA_AUTOVOICE     0x00000002 /* Gain voice automatically upon entry. */
#define CA_OP            0x00000004 /* Ability to use op/deop command. */
#define CA_AUTOOP        0x00000008 /* Gain ops automatically upon entry. */
#define CA_TOPIC         0x00000010 /* Ability to use /msg X topic */
#define CA_SET           0x00000020 /* Ability to use /msg X set */
#define CA_REMOVE        0x00000040 /* Ability to use /msg X kick */
#define CA_INVITE        0x00000080 /* Ability to use /msg X invite */
#define CA_RECOVER       0x00000100 /* Ability to use /msg X recover */
#define CA_FLAGS         0x00000200 /* Ability to write to channel flags table */
#define CA_HALFOP	 0x00000400 /* Ability to use /msg X halfop */
#define CA_AUTOHALFOP	 0x00000800 /* Gain halfops automatically upon entry. */
#define CA_ACLVIEW	 0x00001000 /* Can view access lists */
#define CA_FOUNDER	 0x00002000 /* Is a channel founder */
#define CA_USEPROTECT	 0x00004000 /* Ability to use /msg X protect */
#define CA_USEOWNER	 0x00008000 /* Ability to use /msg X owner */

/*#define CA_SUSPENDED	 0x40000000 * Suspended access entry (not yet implemented) */
#define CA_AKICK         0x80000000 /* Automatic kick */

#define CA_NONE          0x0

/* xOP defaults, compatible with Atheme 0.3 */
#define CA_VOP_DEF       (CA_VOICE | CA_AUTOVOICE | CA_ACLVIEW)
#define CA_HOP_DEF	 (CA_VOICE | CA_HALFOP | CA_AUTOHALFOP | CA_TOPIC | CA_ACLVIEW)
#define CA_AOP_DEF       (CA_VOICE | CA_HALFOP | CA_OP | CA_AUTOOP | CA_TOPIC | CA_ACLVIEW)
#define CA_SOP_DEF       (CA_AOP_DEF | CA_SET | CA_REMOVE | CA_INVITE)

/* special values for founder/successor -- jilles */
/* used in shrike flatfile conversion: */
#define CA_SUCCESSOR_0   (CA_VOICE | CA_OP | CA_AUTOOP | CA_TOPIC | CA_SET | CA_REMOVE | CA_INVITE | CA_RECOVER | CA_FLAGS | CA_HALFOP | CA_ACLVIEW | CA_USEPROTECT)
/* granted to new founder on transfer etc: */
#define CA_FOUNDER_0     (CA_SUCCESSOR_0 | CA_FLAGS | CA_USEOWNER | CA_FOUNDER)
/* granted to founder on new channel: */
#define CA_INITIAL       (CA_FOUNDER_0 | CA_AUTOOP)

/* joining with one of these flags updates used time */
#define CA_USEDUPDATE    (CA_VOICE | CA_OP | CA_AUTOOP | CA_SET | CA_REMOVE | CA_RECOVER | CA_FLAGS | CA_HALFOP | CA_AUTOHALFOP | CA_FOUNDER | CA_USEPROTECT | CA_USEOWNER)
/* "high" privs (for MC_LIMITFLAGS) */
#define CA_HIGHPRIVS     (CA_SET | CA_RECOVER | CA_FLAGS)
#define CA_ALLPRIVS      (CA_VOICE | CA_AUTOVOICE | CA_OP | CA_AUTOOP | CA_TOPIC | CA_SET | CA_REMOVE | CA_INVITE | CA_RECOVER | CA_FLAGS | CA_HALFOP | CA_AUTOHALFOP | CA_ACLVIEW | CA_FOUNDER | CA_USEPROTECT | CA_USEOWNER)
#define CA_ALL_ALL       (CA_ALLPRIVS | CA_AKICK)

/* old CA_ flags */
#define OLD_CA_AOP           (CA_VOICE | CA_OP | CA_AUTOOP | CA_TOPIC)

/* shrike CA_ flags */
#define SHRIKE_CA_VOP           0x00000002
#define SHRIKE_CA_AOP           0x00000004
#define SHRIKE_CA_SOP           0x00000008
#define SHRIKE_CA_FOUNDER       0x00000010
#define SHRIKE_CA_SUCCESSOR     0x00000020

/* struct for account memos */
struct mymemo_ {
	char	 sender[NICKLEN];
	char 	 text[MEMOLEN];
	time_t	 sent;
	unsigned int status;
};

/* memo status flags */
#define MEMO_READ          0x00000001
#define MEMO_CHANNEL       0x00000002

/* account related hooks */
typedef struct {
	mychan_t *mc;
	sourceinfo_t *si;
} hook_channel_req_t;

typedef struct {
	mychan_t *mc;
	myuser_t *mu;
} hook_channel_succession_req_t;

typedef struct {
	union {
		mychan_t *mc;
		myuser_t *mu;
		mynick_t *mn;
	} data;
	int do_expire;	/* Write zero here to disallow expiry */
} hook_expiry_req_t;

typedef struct {
	sourceinfo_t *si;
	const char *name;
	channel_t *chan;
	int approved; /* Write non-zero here to disallow the registration */
} hook_channel_register_check_t;

typedef struct {
	sourceinfo_t *si;
	myuser_t *mu;
	mynick_t *mn;
} hook_user_req_t;

typedef struct {
	sourceinfo_t *si;
	const char *account; /* or nick */
	const char *email;
	const char *password;
	int approved; /* Write non-zero here to disallow the registration */
} hook_user_register_check_t;

typedef struct {
	user_t *u;
	mynick_t *mn;
} hook_nick_enforce_t;

typedef struct {
	myuser_t *target;
	const char *name;
	char *value;
} hook_metadata_change_t;

typedef struct {
	myuser_t *mu;
	const char *oldname;
} hook_user_rename_t;

/* pmodule.c XXX */
E bool backend_loaded;

/* dbhandler.c */
E void (*db_save)(void *arg);
E void (*db_load)(void);

/* function.c */
E bool is_founder(mychan_t *mychan, myentity_t *myuser);

/* node.c */
E mowgli_list_t klnlist;

E kline_t *kline_add(const char *user, const char *host, const char *reason, long duration, const char *setby);
E void kline_delete(kline_t *k);
E kline_t *kline_find(const char *user, const char *host);
E kline_t *kline_find_num(unsigned long number);
E kline_t *kline_find_user(user_t *u);
E void kline_expire(void *arg);

E mowgli_list_t xlnlist;

E xline_t *xline_add(const char *realname, const char *reason, long duration, const char *setby);
E void xline_delete(const char *realname);
E xline_t *xline_find(const char *realname);
E xline_t *xline_find_num(unsigned int number);
E xline_t *xline_find_user(user_t *u);
E void xline_expire(void *arg);

E mowgli_list_t qlnlist;

E qline_t *qline_add(const char *mask, const char *reason, long duration, const char *setby);
E void qline_delete(const char *mask);
E qline_t *qline_find(const char *mask);
E qline_t *qline_find_num(unsigned int number);
E qline_t *qline_find_user(user_t *u);
E qline_t *qline_find_channel(channel_t *c);
E void qline_expire(void *arg);

/* account.c */
E mowgli_patricia_t *nicklist;
E mowgli_patricia_t *oldnameslist;
E mowgli_patricia_t *mclist;

E void init_accounts(void);

E myuser_t *myuser_add(const char *name, const char *pass, const char *email, unsigned int flags);
E void myuser_delete(myuser_t *mu);
//inline myuser_t *myuser_find(const char *name);
E void myuser_rename(myuser_t *mu, const char *name);
E void myuser_set_email(myuser_t *mu, const char *newemail);
E myuser_t *myuser_find_ext(const char *name);
E void myuser_notice(const char *from, myuser_t *target, const char *fmt, ...) PRINTFLIKE(3, 4);

E bool myuser_access_verify(user_t *u, myuser_t *mu);
E bool myuser_access_add(myuser_t *mu, const char *mask);
E char *myuser_access_find(myuser_t *mu, const char *mask);
E void myuser_access_delete(myuser_t *mu, const char *mask);

E mynick_t *mynick_add(myuser_t *mu, const char *name);
E void mynick_delete(mynick_t *mn);
//inline mynick_t *mynick_find(const char *name);

E myuser_name_t *myuser_name_add(const char *name);
//inline myuser_name_t *myuser_name_find(const char *name);
E void myuser_name_remember(const char *name, myuser_t *mu);
E void myuser_name_restore(const char *name, myuser_t *mu);

E mycertfp_t *mycertfp_add(myuser_t *mu, const char *certfp);
E void mycertfp_delete(mycertfp_t *mcfp);
E mycertfp_t *mycertfp_find(const char *certfp);

E mychan_t *mychan_add(char *name);
//inline mychan_t *mychan_find(const char *name);
E bool mychan_isused(mychan_t *mc);
E unsigned int mychan_num_founders(mychan_t *mc);
E const char *mychan_founder_names(mychan_t *mc);
E myuser_t *mychan_pick_candidate(mychan_t *mc, unsigned int minlevel);
E myuser_t *mychan_pick_successor(mychan_t *mc);
E const char *mychan_get_mlock(mychan_t *mc);
E const char *mychan_get_sts_mlock(mychan_t *mc);

E chanacs_t *chanacs_add(mychan_t *mychan, myentity_t *myuser, unsigned int level, time_t ts);
E chanacs_t *chanacs_add_host(mychan_t *mychan, const char *host, unsigned int level, time_t ts);

E chanacs_t *chanacs_find(mychan_t *mychan, myentity_t *myuser, unsigned int level);
E chanacs_t *chanacs_find_literal(mychan_t *mychan, myentity_t *myuser, unsigned int level);
E chanacs_t *chanacs_find_host(mychan_t *mychan, const char *host, unsigned int level);
E unsigned int chanacs_host_flags(mychan_t *mychan, const char *host);
E chanacs_t *chanacs_find_host_literal(mychan_t *mychan, const char *host, unsigned int level);
E chanacs_t *chanacs_find_host_by_user(mychan_t *mychan, user_t *u, unsigned int level);
E unsigned int chanacs_host_flags_by_user(mychan_t *mychan, user_t *u);
E chanacs_t *chanacs_find_by_mask(mychan_t *mychan, const char *mask, unsigned int level);
E bool chanacs_user_has_flag(mychan_t *mychan, user_t *u, unsigned int level);
E unsigned int chanacs_user_flags(mychan_t *mychan, user_t *u);
//inline bool chanacs_source_has_flag(mychan_t *mychan, sourceinfo_t *si, unsigned int level);
E unsigned int chanacs_source_flags(mychan_t *mychan, sourceinfo_t *si);

E chanacs_t *chanacs_open(mychan_t *mychan, myentity_t *mt, const char *hostmask, bool create);
//inline void chanacs_close(chanacs_t *ca);
E bool chanacs_modify(chanacs_t *ca, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags);
E bool chanacs_modify_simple(chanacs_t *ca, unsigned int addflags, unsigned int removeflags);

//inline bool chanacs_is_table_full(chanacs_t *ca);

E bool chanacs_change(mychan_t *mychan, myentity_t *mt, const char *hostmask, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags);
E bool chanacs_change_simple(mychan_t *mychan, myentity_t *mt, const char *hostmask, unsigned int addflags, unsigned int removeflags);

E void expire_check(void *arg);
/* Check the database for (version) problems common to all backends */
E void db_check(void);

/* svsignore.c */
E mowgli_list_t svs_ignore_list;

E svsignore_t *svsignore_find(user_t *user);
E svsignore_t *svsignore_add(const char *mask, const char *reason);
E void svsignore_delete(svsignore_t *svsignore);

#include "entity-validation.h"

#endif

// vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
