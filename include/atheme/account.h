/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
 *
 * Data structures for account information.
 */

#ifndef ATHEME_INC_ACCOUNT_H
#define ATHEME_INC_ACCOUNT_H 1

#include <atheme/attributes.h>
#include <atheme/entity.h>
#include <atheme/object.h>
#include <atheme/stdheaders.h>
#include <atheme/structures.h>

/* kline list struct */
struct kline
{
	char *          user;
	char *          host;
	char *          reason;
	char *          setby;
	unsigned long   number;
	long            duration;
	time_t          settime;
	time_t          expires;
};

/* xline list struct */
struct xline
{
	char *          realname;
	char *          reason;
	char *          setby;
	unsigned int    number;
	long            duration;
	time_t          settime;
	time_t          expires;
};

/* qline list struct */
struct qline
{
	char *          mask;
	char *          reason;
	char *          setby;
	unsigned int    number;
	long            duration;
	time_t          settime;
	time_t          expires;
};

/* services ignore struct */
struct svsignore
{
	struct svsignore *      svsignore;
	char *                  mask;
	time_t                  settime;
	char *                  setby;
	char *                  reason;
};

/* services accounts */
struct myuser
{
	struct myentity         ent;
	char                    pass[PASSLEN + 1];
	stringref               email;
	stringref               email_canonical;
	mowgli_list_t           logins;                 // 'struct user's currently logged in to this
	time_t                  registered;
	time_t                  lastlogin;
	struct soper *          soper;
	unsigned int            flags;
	mowgli_list_t           memos;                  // store memos
	unsigned int            memoct_new;
	unsigned int            memo_ratelimit_num;     // memos sent recently
	time_t                  memo_ratelimit_time;    // last time a memo was sent
	mowgli_list_t           memo_ignores;
	mowgli_list_t           access_list;
	mowgli_list_t           nicks;                  // registered nicks, must include mu->name if nonempty
	struct language *       language;
	mowgli_list_t           cert_fingerprints;
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
#define MU_LOGINNOLIMIT 0x00100000U

/* memoserv rate limiting parameters */
#define MEMO_MAX_NUM   5
#define MEMO_MAX_TIME  180

/* registered nick */
struct mynick
{
	struct atheme_object    parent;
	char                    nick[NICKLEN + 1];
	struct myuser *         owner;
	time_t                  registered;
	time_t                  lastseen;
	mowgli_node_t           node;   // for struct myuser -> nicks
};

/* record about a name that used to exist */
struct myuser_name
{
	struct atheme_object    parent;
	char                    name[NICKLEN + 1];
};

struct mycertfp
{
	struct myuser * mu;
	char *          certfp;
	mowgli_node_t   node;
};

struct mychan
{
	struct atheme_object    parent;
	stringref               name;
	struct channel *        chan;
	mowgli_list_t           chanacs;
	time_t                  registered;
	time_t                  used;
	unsigned int            mlock_on;
	unsigned int            mlock_off;
	unsigned int            mlock_limit;
	char *                  mlock_key;
	unsigned int            flags;
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
struct chanacs
{
	struct atheme_object    parent;
	struct myentity *       entity;
	struct mychan *         mychan;
	char *                  host;
	unsigned int            level;
	time_t                  tmodified;
	mowgli_node_t           cnode;
	mowgli_node_t           unode;
	char                    setter_uid[IDLEN + 1];
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
#define CA_SOP_DEF       (CA_AOP_DEF | CA_SET | CA_REMOVE | CA_INVITE | CA_EXEMPT)

/* special values for founder/successor -- jilles */
/* used in shrike flatfile conversion: */
#define CA_SUCCESSOR_0   (CA_VOICE | CA_OP | CA_AUTOOP | CA_TOPIC | CA_SET | CA_REMOVE | CA_INVITE | CA_RECOVER | CA_FLAGS | CA_HALFOP | CA_ACLVIEW | CA_USEPROTECT | CA_EXEMPT)
/* granted to new founder on transfer etc: */
#define CA_FOUNDER_0     (CA_SUCCESSOR_0 | CA_FLAGS | CA_USEOWNER | CA_FOUNDER)
/* granted to founder on new channel by default: */
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

struct mygroup
{
	struct myentity ent;
	mowgli_list_t   acs;
	time_t          regtime;
	unsigned int    flags;
	bool            visited;
};

#define MG_REGNOLIMIT		0x00000001U
#define MG_ACSNOLIMIT		0x00000002U
#define MG_OPEN			0x00000004U
#define MG_PUBLIC		0x00000008U

struct groupacs
{
	struct atheme_object    parent;
	struct mygroup *        mg;
	struct myentity *       mt;
	unsigned int            flags;
	mowgli_node_t           gnode;
	mowgli_node_t           unode;
};

#define GA_FOUNDER		0x00000001U
#define GA_FLAGS		0x00000002U
#define GA_CHANACS		0x00000004U
#define GA_MEMOS		0x00000008U
#define GA_SET			0x00000010U
#define GA_VHOST		0x00000020U
#define GA_BAN			0x00000040U
#define GA_INVITE		0x00000080U
#define GA_ACLVIEW		0x00000100U
#define GA_ALL			(GA_FLAGS | GA_CHANACS | GA_MEMOS | GA_SET | GA_VHOST | GA_INVITE | GA_ACLVIEW)
#define GA_ALL_OLD		(GA_FLAGS | GA_CHANACS | GA_MEMOS | GA_SET | GA_VHOST | GA_INVITE)

/* struct for account memos */
struct mymemo
{
	char            sender[NICKLEN + 1];
	char            text[MEMOLEN + 1];
	time_t          sent;
	unsigned int    status;
};

/* memo status flags */
#define MEMO_READ          0x00000001U
#define MEMO_CHANNEL       0x00000002U

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
bool is_founder(struct mychan *mychan, struct myentity *myuser);

/* node.c */
extern mowgli_list_t klnlist;

struct kline *kline_add_with_id(const char *user, const char *host, const char *reason, long duration, const char *setby, unsigned long id);
struct kline *kline_add(const char *user, const char *host, const char *reason, long duration, const char *setby);
struct kline *kline_add_user(struct user *user, const char *reason, long duration, const char *setby);
void kline_delete(struct kline *k);
struct kline *kline_find(const char *user, const char *host);
struct kline *kline_find_num(unsigned long number);
struct kline *kline_find_user(struct user *u);
void kline_expire(void *arg);

extern mowgli_list_t xlnlist;

struct xline *xline_add(const char *realname, const char *reason, long duration, const char *setby);
void xline_delete(const char *realname);
struct xline *xline_find(const char *realname);
struct xline *xline_find_num(unsigned int number);
struct xline *xline_find_user(struct user *u);
void xline_expire(void *arg);

extern mowgli_list_t qlnlist;

struct qline *qline_add(const char *mask, const char *reason, long duration, const char *setby);
void qline_delete(const char *mask);
struct qline *qline_find(const char *mask);
struct qline *qline_find_match(const char *mask);
struct qline *qline_find_num(unsigned int number);
struct qline *qline_find_user(struct user *u);
struct qline *qline_find_channel(struct channel *c);
void qline_expire(void *arg);

/* account.c */
extern mowgli_patricia_t *nicklist;
extern mowgli_patricia_t *oldnameslist;
extern mowgli_patricia_t *mclist;

void init_accounts(void);

struct myuser *myuser_add(const char *name, const char *pass, const char *email, unsigned int flags);
struct myuser *myuser_add_id(const char *id, const char *name, const char *pass, const char *email, unsigned int flags);
void myuser_delete(struct myuser *mu);
//inline struct myuser *myuser_find(const char *name);
void myuser_rename(struct myuser *mu, const char *name);
void myuser_set_email(struct myuser *mu, const char *newemail);
struct myuser *myuser_find_ext(const char *name);
void myuser_notice(const char *from, struct myuser *target, const char *fmt, ...) ATHEME_FATTR_PRINTF(3, 4);

bool myuser_access_verify(struct user *u, struct myuser *mu);
bool myuser_access_add(struct myuser *mu, const char *mask);
char *myuser_access_find(struct myuser *mu, const char *mask);
void myuser_access_delete(struct myuser *mu, const char *mask);

struct mynick *mynick_add(struct myuser *mu, const char *name);
void mynick_delete(struct mynick *mn);

struct myuser_name *myuser_name_add(const char *name);
void myuser_name_remember(const char *name, struct myuser *mu);
void myuser_name_restore(const char *name, struct myuser *mu);

struct mycertfp *mycertfp_add(struct myuser *mu, const char *certfp, bool force);
void mycertfp_delete(struct mycertfp *mcfp);
struct mycertfp *mycertfp_find(const char *certfp);

struct mychan *mychan_add(char *name);
//inline struct mychan *mychan_find(const char *name);
bool mychan_isused(struct mychan *mc);
unsigned int mychan_num_founders(struct mychan *mc);
const char *mychan_founder_names(struct mychan *mc);
struct myuser *mychan_pick_candidate(struct mychan *mc, unsigned int minlevel);
struct myuser *mychan_pick_successor(struct mychan *mc);
const char *mychan_get_mlock(struct mychan *mc);
const char *mychan_get_sts_mlock(struct mychan *mc);

struct chanacs *chanacs_add(struct mychan *mychan, struct myentity *myuser, unsigned int level, time_t ts, struct myentity *setter);
struct chanacs *chanacs_add_host(struct mychan *mychan, const char *host, unsigned int level, time_t ts, struct myentity *setter);

struct chanacs *chanacs_find(struct mychan *mychan, struct myentity *myuser, unsigned int level);
unsigned int chanacs_entity_flags(struct mychan *mychan, struct myentity *myuser);
struct chanacs *chanacs_find_literal(struct mychan *mychan, struct myentity *myuser, unsigned int level);
struct chanacs *chanacs_find_host(struct mychan *mychan, const char *host, unsigned int level);
unsigned int chanacs_host_flags(struct mychan *mychan, const char *host);
struct chanacs *chanacs_find_host_literal(struct mychan *mychan, const char *host, unsigned int level);
struct chanacs *chanacs_find_host_by_user(struct mychan *mychan, struct user *u, unsigned int level);
struct chanacs *chanacs_find_by_mask(struct mychan *mychan, const char *mask, unsigned int level);
bool chanacs_user_has_flag(struct mychan *mychan, struct user *u, unsigned int level);
unsigned int chanacs_user_flags(struct mychan *mychan, struct user *u);
//inline bool chanacs_source_has_flag(struct mychan *mychan, struct sourceinfo *si, unsigned int level);
unsigned int chanacs_source_flags(struct mychan *mychan, struct sourceinfo *si);

struct chanacs *chanacs_open(struct mychan *mychan, struct myentity *mt, const char *hostmask, bool create, struct myentity *setter);
//inline void chanacs_close(struct chanacs *ca);
bool chanacs_modify(struct chanacs *ca, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags, struct myuser *setter);
bool chanacs_modify_simple(struct chanacs *ca, unsigned int addflags, unsigned int removeflags, struct myuser *setter);

//inline bool chanacs_is_table_full(struct chanacs *ca);

bool chanacs_change(struct mychan *mychan, struct myentity *mt, const char *hostmask, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags, struct myentity *setter);
bool chanacs_change_simple(struct mychan *mychan, struct myentity *mt, const char *hostmask, unsigned int addflags, unsigned int removeflags, struct myentity *setter);

void expire_check(void *arg);
/* Check the database for (version) problems common to all backends */
void db_check(void);

/* svsignore.c */
extern mowgli_list_t svs_ignore_list;

struct svsignore *svsignore_find(struct user *user);
struct svsignore *svsignore_add(const char *mask, const char *reason) ATHEME_FATTR_MALLOC ATHEME_FATTR_RETURNS_NONNULL;
void svsignore_delete(struct svsignore *svsignore);

/* check if user is at the login limit or if they are exempt */
bool user_loginmaxed(struct myuser *mu);

#endif /* !ATHEME_INC_ACCOUNT_H */
