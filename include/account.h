/*
 * Copyright (c) 2005-2009 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for account information.
 */

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "entity.h"

typedef struct chanacs_ chanacs_t;
typedef struct svsignore_ svsignore_t;

/* kline list struct */
struct kline
{
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
struct xline
{
  char *realname;
  char *reason;
  char *setby;

  unsigned int number;
  long duration;
  time_t settime;
  time_t expires;
};

/* qline list struct */
struct qline
{
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
  char pass[PASSLEN + 1];

  stringref email;
  stringref email_canonical;

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

  mowgli_list_t access_list;
  mowgli_list_t nicks; /* registered nicks, must include mu->name if nonempty */

  language_t *language;

  mowgli_list_t cert_fingerprints;
};

/* Keep this synchronized with mu_flags in libathemecore/flags.c */
#define MU_HOLD         0x00000001U
#define MU_NEVEROP      0x00000002U
#define MU_NOOP         0x00000004U
#define MU_WAITAUTH     0x00000008U
#define MU_HIDEMAIL     0x00000010U
#define MU_NOMEMO       0x00000040U
#define MU_EMAILMEMOS   0x00000080U
#define MU_CRYPTPASS    0x00000100U
#define MU_NOBURSTLOGIN 0x00000400U
#define MU_ENFORCE      0x00000800U /* XXX limited use at this time */
#define MU_USE_PRIVMSG  0x00001000U /* use PRIVMSG */
#define MU_PRIVATE      0x00002000U
#define MU_QUIETCHG     0x00004000U
#define MU_NOGREET      0x00008000U
#define MU_REGNOLIMIT   0x00010000U
#define MU_NEVERGROUP   0x00020000U
#define MU_PENDINGLOGIN 0x00040000U
#define MU_NOPASSWORD   0x00080000U

/* memoserv rate limiting parameters */
#define MEMO_MAX_NUM   5
#define MEMO_MAX_TIME  180

/* registered nick */
struct mynick_
{
  object_t parent;

  char nick[NICKLEN + 1];

  myuser_t *owner;

  time_t registered;
  time_t lastseen;

  mowgli_node_t node; /* for myuser_t.nicks */
};

/* record about a name that used to exist */
struct myuser_name
{
  object_t parent;

  char name[NICKLEN + 1];
};

struct mycertfp
{
  myuser_t *mu;

  char *certfp;

  mowgli_node_t node;
};

struct mychan_
{
  object_t parent;

  stringref name;

  struct channel *chan;
  mowgli_list_t chanacs;
  time_t registered;
  time_t used;

  unsigned int mlock_on;
  unsigned int mlock_off;
  unsigned int mlock_limit;
  char *mlock_key;

  unsigned int flags;
};

/* Keep this synchronized with mc_flags in libathemecore/flags.c */
#define MC_HOLD         0x00000001U
#define MC_NOOP         0x00000002U
#define MC_LIMITFLAGS   0x00000004U
#define MC_SECURE       0x00000008U
#define MC_VERBOSE      0x00000010U
#define MC_RESTRICTED   0x00000020U
#define MC_KEEPTOPIC    0x00000040U
#define MC_VERBOSE_OPS  0x00000080U
#define MC_TOPICLOCK    0x00000100U
#define MC_GUARD        0x00000200U
#define MC_PRIVATE      0x00000400U
#define MC_NOSYNC       0x00000800U
#define MC_ANTIFLOOD    0x00001000U
#define MC_PUBACL       0x00002000U

/* The following are temporary state */
#define MC_INHABIT      0x80000000U /* we're on channel to enforce akick/staffonly/close */
#define MC_MLOCK_CHECK  0x40000000U /* we need to check mode locks */
#define MC_FORCEVERBOSE 0x20000000U /* fantasy cmd in progress, be verbose */
#define MC_RECREATED    0x10000000U /* created with new channelTS */

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
	mowgli_node_t    unode;

	char setter_uid[IDLEN + 1];
};

/* the new atheme-style channel flags */
#define CA_VOICE         0x00000001U /* Ability to use voice/devoice command. */
#define CA_AUTOVOICE     0x00000002U /* Gain voice automatically upon entry. */
#define CA_OP            0x00000004U /* Ability to use op/deop command. */
#define CA_AUTOOP        0x00000008U /* Gain ops automatically upon entry. */
#define CA_TOPIC         0x00000010U /* Ability to use /msg X topic */
#define CA_SET           0x00000020U /* Ability to use /msg X set */
#define CA_REMOVE        0x00000040U /* Ability to use /msg X kick */
#define CA_INVITE        0x00000080U /* Ability to use /msg X invite */
#define CA_RECOVER       0x00000100U /* Ability to use /msg X recover */
#define CA_FLAGS         0x00000200U /* Ability to write to channel flags table */
#define CA_HALFOP        0x00000400U /* Ability to use /msg X halfop */
#define CA_AUTOHALFOP    0x00000800U /* Gain halfops automatically upon entry. */
#define CA_ACLVIEW       0x00001000U /* Can view access lists */
#define CA_FOUNDER       0x00002000U /* Is a channel founder */
#define CA_USEPROTECT    0x00004000U /* Ability to use /msg X protect */
#define CA_USEOWNER      0x00008000U /* Ability to use /msg X owner */
#define CA_EXEMPT        0x00010000U /* Exempt from akick, can use /msg X unban on self */

/*#define CA_SUSPENDED     0x40000000U * Suspended access entry (not yet implemented) */
#define CA_AKICK         0x80000000U /* Automatic kick */

#define CA_NONE          0x0U

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
#define CA_ALLPRIVS      (CA_VOICE | CA_AUTOVOICE | CA_OP | CA_AUTOOP | CA_TOPIC | CA_SET | CA_REMOVE | CA_INVITE | CA_RECOVER | CA_FLAGS | CA_HALFOP | CA_AUTOHALFOP | CA_ACLVIEW | CA_FOUNDER | CA_USEPROTECT | CA_USEOWNER | CA_EXEMPT)
#define CA_ALL_ALL       (CA_ALLPRIVS | CA_AKICK)

/* old CA_ flags */
#define OLD_CA_AOP           (CA_VOICE | CA_OP | CA_AUTOOP | CA_TOPIC)

/* shrike CA_ flags */
#define SHRIKE_CA_VOP           0x00000002U
#define SHRIKE_CA_AOP           0x00000004U
#define SHRIKE_CA_SOP           0x00000008U
#define SHRIKE_CA_FOUNDER       0x00000010U
#define SHRIKE_CA_SUCCESSOR     0x00000020U

/* struct for account memos */
struct mymemo
{
	char	 sender[NICKLEN + 1];
	char 	 text[MEMOLEN + 1];
	time_t	 sent;
	unsigned int status;
};

/* memo status flags */
#define MEMO_READ          0x00000001U
#define MEMO_CHANNEL       0x00000002U

/* account related hooks */
typedef struct {
	mychan_t *mc;
	struct sourceinfo *si;
} hook_channel_req_t;

typedef struct {
	chanacs_t *ca;
	struct sourceinfo *si;
	myentity_t *parent;
	unsigned int oldlevel;
	unsigned int newlevel;
	int approved;
} hook_channel_acl_req_t;

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
	struct sourceinfo *si;
	const char *name;
	struct channel *chan;
	int approved; /* Write non-zero here to disallow the registration */
} hook_channel_register_check_t;

typedef struct {
	struct sourceinfo *si;
	myuser_t *mu;
	mynick_t *mn;
} hook_user_req_t;

typedef struct {
	struct sourceinfo *si;
	const char *account; /* or nick */
	const char *email;
	const char *password;
	int approved; /* Write non-zero here to disallow the registration */
} hook_user_register_check_t;

typedef struct {
	struct sourceinfo *si;
	myuser_t *mu;
	bool allowed;
} hook_user_login_check_t;

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

typedef struct {
	struct sourceinfo *si;
	const char *nick;
} hook_info_noexist_req_t;

typedef struct {
	struct sourceinfo *si;
	myuser_t *mu;
	int allowed;
} hook_user_needforce_t;

/* pmodule.c XXX */
extern bool backend_loaded;

/* dbhandler.c */
/* BLOCKING:     wait for the write to finish; cancel previous write if necessary
 * BG_REGULAR:   try to fork, no-op if a previous write is still in progress
 * BG_IMPORTANT: try to fork, canceling previous write first if necessary
 */
enum db_save_strategy
{
	DB_SAVE_BLOCKING,
	DB_SAVE_BG_REGULAR,
	DB_SAVE_BG_IMPORTANT
};

extern void (*db_save)(void *arg, enum db_save_strategy strategy);
extern void (*db_load)(const char *arg);

/* function.c */
extern bool is_founder(mychan_t *mychan, myentity_t *myuser);

/* node.c */
extern mowgli_list_t klnlist;

extern struct kline *kline_add_with_id(const char *user, const char *host, const char *reason, long duration, const char *setby, unsigned long id);
extern struct kline *kline_add(const char *user, const char *host, const char *reason, long duration, const char *setby);
extern struct kline *kline_add_user(user_t *user, const char *reason, long duration, const char *setby);
extern void kline_delete(struct kline *k);
extern struct kline *kline_find(const char *user, const char *host);
extern struct kline *kline_find_num(unsigned long number);
extern struct kline *kline_find_user(user_t *u);
extern void kline_expire(void *arg);

extern mowgli_list_t xlnlist;

extern struct xline *xline_add(const char *realname, const char *reason, long duration, const char *setby);
extern void xline_delete(const char *realname);
extern struct xline *xline_find(const char *realname);
extern struct xline *xline_find_num(unsigned int number);
extern struct xline *xline_find_user(user_t *u);
extern void xline_expire(void *arg);

extern mowgli_list_t qlnlist;

extern struct qline *qline_add(const char *mask, const char *reason, long duration, const char *setby);
extern void qline_delete(const char *mask);
extern struct qline *qline_find(const char *mask);
extern struct qline *qline_find_match(const char *mask);
extern struct qline *qline_find_num(unsigned int number);
extern struct qline *qline_find_user(user_t *u);
extern struct qline *qline_find_channel(struct channel *c);
extern void qline_expire(void *arg);

/* account.c */
extern mowgli_patricia_t *nicklist;
extern mowgli_patricia_t *oldnameslist;
extern mowgli_patricia_t *mclist;

extern void init_accounts(void);

extern myuser_t *myuser_add(const char *name, const char *pass, const char *email, unsigned int flags);
extern myuser_t *myuser_add_id(const char *id, const char *name, const char *pass, const char *email, unsigned int flags);
extern void myuser_delete(myuser_t *mu);
//inline myuser_t *myuser_find(const char *name);
extern void myuser_rename(myuser_t *mu, const char *name);
extern void myuser_set_email(myuser_t *mu, const char *newemail);
extern myuser_t *myuser_find_ext(const char *name);
extern void myuser_notice(const char *from, myuser_t *target, const char *fmt, ...) ATHEME_FATTR_PRINTF(3, 4);

extern bool myuser_access_verify(user_t *u, myuser_t *mu);
extern bool myuser_access_add(myuser_t *mu, const char *mask);
extern char *myuser_access_find(myuser_t *mu, const char *mask);
extern void myuser_access_delete(myuser_t *mu, const char *mask);

extern mynick_t *mynick_add(myuser_t *mu, const char *name);
extern void mynick_delete(mynick_t *mn);
//inline mynick_t *mynick_find(const char *name);

extern struct myuser_name *myuser_name_add(const char *name);
extern void myuser_name_remember(const char *name, myuser_t *mu);
extern void myuser_name_restore(const char *name, myuser_t *mu);

extern struct mycertfp *mycertfp_add(myuser_t *mu, const char *certfp);
extern void mycertfp_delete(struct mycertfp *mcfp);
extern struct mycertfp *mycertfp_find(const char *certfp);

extern mychan_t *mychan_add(char *name);
//inline mychan_t *mychan_find(const char *name);
extern bool mychan_isused(mychan_t *mc);
extern unsigned int mychan_num_founders(mychan_t *mc);
extern const char *mychan_founder_names(mychan_t *mc);
extern myuser_t *mychan_pick_candidate(mychan_t *mc, unsigned int minlevel);
extern myuser_t *mychan_pick_successor(mychan_t *mc);
extern const char *mychan_get_mlock(mychan_t *mc);
extern const char *mychan_get_sts_mlock(mychan_t *mc);

extern chanacs_t *chanacs_add(mychan_t *mychan, myentity_t *myuser, unsigned int level, time_t ts, myentity_t *setter);
extern chanacs_t *chanacs_add_host(mychan_t *mychan, const char *host, unsigned int level, time_t ts, myentity_t *setter);

extern chanacs_t *chanacs_find(mychan_t *mychan, myentity_t *myuser, unsigned int level);
extern unsigned int chanacs_entity_flags(mychan_t *mychan, myentity_t *myuser);
//inline bool chanacs_entity_has_flag(mychan_t *mychan, myentity_t *mt, unsigned int level)
extern chanacs_t *chanacs_find_literal(mychan_t *mychan, myentity_t *myuser, unsigned int level);
extern chanacs_t *chanacs_find_host(mychan_t *mychan, const char *host, unsigned int level);
extern unsigned int chanacs_host_flags(mychan_t *mychan, const char *host);
extern chanacs_t *chanacs_find_host_literal(mychan_t *mychan, const char *host, unsigned int level);
extern chanacs_t *chanacs_find_host_by_user(mychan_t *mychan, user_t *u, unsigned int level);
extern chanacs_t *chanacs_find_by_mask(mychan_t *mychan, const char *mask, unsigned int level);
extern bool chanacs_user_has_flag(mychan_t *mychan, user_t *u, unsigned int level);
extern unsigned int chanacs_user_flags(mychan_t *mychan, user_t *u);
//inline bool chanacs_source_has_flag(mychan_t *mychan, struct sourceinfo *si, unsigned int level);
extern unsigned int chanacs_source_flags(mychan_t *mychan, struct sourceinfo *si);

extern chanacs_t *chanacs_open(mychan_t *mychan, myentity_t *mt, const char *hostmask, bool create, myentity_t *setter);
//inline void chanacs_close(chanacs_t *ca);
extern bool chanacs_modify(chanacs_t *ca, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags, myuser_t *setter);
extern bool chanacs_modify_simple(chanacs_t *ca, unsigned int addflags, unsigned int removeflags, myuser_t *setter);

//inline bool chanacs_is_table_full(chanacs_t *ca);

extern bool chanacs_change(mychan_t *mychan, myentity_t *mt, const char *hostmask, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags, myentity_t *setter);
extern bool chanacs_change_simple(mychan_t *mychan, myentity_t *mt, const char *hostmask, unsigned int addflags, unsigned int removeflags, myentity_t *setter);

extern void expire_check(void *arg);
/* Check the database for (version) problems common to all backends */
extern void db_check(void);

/* svsignore.c */
extern mowgli_list_t svs_ignore_list;

extern svsignore_t *svsignore_find(user_t *user);
extern svsignore_t *svsignore_add(const char *mask, const char *reason);
extern void svsignore_delete(svsignore_t *svsignore);

#include "entity-validation.h"

#endif
