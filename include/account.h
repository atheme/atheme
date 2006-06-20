/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for account information.
 *
 * $Id: account.h 5460 2006-06-20 19:08:22Z jilles $
 */

#ifndef ACCOUNT_H
#define ACCOUNT_H

typedef struct operclass_ operclass_t;
typedef struct soper_ soper_t;
typedef struct svsignore_ svsignore_t;
typedef struct myuser_ myuser_t;
typedef struct mychan_ mychan_t;
typedef struct chanacs_ chanacs_t;
typedef struct mymemo_ mymemo_t;

struct operclass_ {
  char *name;
  char *privs; /* priv1 priv2 priv3... */
};

/* soper list struct */
struct soper_ {
  myuser_t *myuser;
  char *name;
  operclass_t *operclass;
};

/* services ignore struct */
struct svsignore_ {
  svsignore_t *svsignore;

  char *mask;
  time_t settime;
  char *setby;
  char *reason;
};

struct myuser_
{
  char name[NICKLEN];
  char pass[NICKLEN];
  char email[EMAILLEN];

  list_t logins; /* user_t's currently logged in to this */
  time_t registered;
  time_t lastlogin;

  list_t chanacs;
  soper_t *soper;

  list_t metadata;

  uint32_t flags;
  int32_t hash;
  
  list_t memos; /* store memos */
  uint8_t memoct_new;
  uint8_t memo_ratelimit_num; /* memos sent recently */
  time_t memo_ratelimit_time; /* last time a memo was sent */
  list_t memo_ignores;
};

#define MU_HOLD        0x00000001
#define MU_NEVEROP     0x00000002
#define MU_NOOP        0x00000004
#define MU_WAITAUTH    0x00000008
#define MU_HIDEMAIL    0x00000010
#define MU_OLD_ALIAS   0x00000020 /* obsolete */
#define MU_NOMEMO      0x00000040
#define MU_EMAILMEMOS  0x00000080
#define MU_CRYPTPASS   0x00000100
#define MU_SASL        0x00000200 /* yuck, but sadly the best way */

/* memoserv rate limiting parameters */
#define MEMO_MAX_NUM   5
#define MEMO_MAX_TIME  180

struct mychan_
{
  char name[CHANNELLEN];

  myuser_t *founder;
  channel_t *chan;
  list_t chanacs;
  time_t registered;
  time_t used;

  int mlock_on;
  int mlock_off;
  uint32_t mlock_limit;
  char *mlock_key;

  uint32_t flags;
  int32_t hash;

  list_t metadata;
};

#define MC_HOLD        0x00000001
#define MC_NOOP        0x00000002
#define MC_RESTRICTED  0x00000004
#define MC_SECURE      0x00000008
#define MC_VERBOSE     0x00000010
#define MC_STAFFONLY   0x00000020
#define MC_KEEPTOPIC   0x00000040
#define MC_VERBOSE_OPS 0x00000080

/* The following are temporary state */
#define MC_INHABIT     0x80000000 /* we're on channel to enforce akick/staffonly/close */
#define MC_MLOCK_CHECK 0x40000000 /* we need to check mode locks */

/* struct for channel access list */
struct chanacs_
{
	myuser_t *myuser;
	mychan_t *mychan;
	char      host[HOSTLEN];
	uint32_t  level;
	list_t	  metadata;
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

#define CA_AKICK         0x80000000 /* Automatic kick */

#define CA_NONE          0x0

/* xOP defaults, compatible with Atheme 0.3 */
#define CA_VOP_DEF       (CA_VOICE | CA_AUTOVOICE | CA_ACLVIEW)
#define CA_HOP_DEF	 (CA_VOICE | CA_HALFOP | CA_AUTOHALFOP | CA_TOPIC | CA_ACLVIEW)
#define CA_AOP_DEF       (CA_VOICE | CA_HALFOP | CA_OP | CA_AUTOOP | CA_TOPIC | CA_ACLVIEW)
#define CA_SOP_DEF       (CA_AOP_DEF | CA_SET | CA_REMOVE | CA_INVITE)

/* special values for founder/successor -- jilles */
/* used in shrike flatfile conversion: */
#define CA_SUCCESSOR_0   (CA_VOICE | CA_OP | CA_AUTOOP | CA_TOPIC | CA_SET | CA_REMOVE | CA_INVITE | CA_RECOVER | CA_FLAGS | CA_HALFOP | CA_ACLVIEW)
/* granted to new founder on transfer etc: */
#define CA_FOUNDER_0     (CA_SUCCESSOR_0 | CA_FLAGS)
/* granted to founder on new channel: */
#define CA_INITIAL       (CA_FOUNDER_0 | CA_AUTOOP)

/* joining with one of these flags updates used time */
#define CA_USEDUPDATE    (CA_VOICE | CA_OP | CA_AUTOOP | CA_SET | CA_REMOVE | CA_RECOVER | CA_FLAGS | CA_HALFOP | CA_AUTOHALFOP)
#define CA_ALLPRIVS      (CA_VOICE | CA_AUTOVOICE | CA_OP | CA_AUTOOP | CA_TOPIC | CA_SET | CA_REMOVE | CA_INVITE | CA_RECOVER | CA_FLAGS | CA_HALFOP | CA_AUTOHALFOP | CA_ACLVIEW)
#define CA_ALL           (CA_ALLPRIVS | CA_AKICK)

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
	uint32_t status;
	list_t	 metadata;
};

/* memo status flags */
#define MEMO_NEW           0x00000000
#define MEMO_READ          0x00000001

#endif
