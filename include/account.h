/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for account information.
 *
 * $Id: account.h 2597 2005-10-05 06:37:06Z kog $
 */

#ifndef ACCOUNT_H
#define ACCOUNT_H

typedef struct sra_ sra_t;
typedef struct myuser_ myuser_t;
typedef struct mychan_ mychan_t;
typedef struct chanacs_ chanacs_t;
typedef struct mymemo_ mymemo_t;

/* sra list struct */
struct sra_ {
  myuser_t *myuser;
  char *name;
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
  sra_t *sra;

  list_t metadata;

  uint32_t flags;
  int32_t hash;
  
  list_t memos; /* store memos */
};

#define MU_HOLD        0x00000001
#define MU_NEVEROP     0x00000002
#define MU_NOOP        0x00000004
#define MU_WAITAUTH    0x00000008
#define MU_HIDEMAIL    0x00000010
#define MU_ALIAS       0x00000020

#define MU_IRCOP       0x00001000
#define MU_SRA         0x00002000

struct mychan_
{
  char name[NICKLEN];

  myuser_t *founder;
  myuser_t *successor;
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

/* compatibility shims for current shrike code. */
#define CA_NONE          0x0
#define CA_VOP           (CA_VOICE | CA_AUTOVOICE | CA_ACLVIEW)
#define CA_HOP		 (CA_VOICE | CA_HALFOP | CA_AUTOHALFOP | CA_TOPIC | CA_ACLVIEW)
#define CA_AOP           (CA_VOICE | CA_HALFOP | CA_OP | CA_AUTOOP | CA_TOPIC | CA_ACLVIEW)
#define CA_SOP           (CA_AOP | CA_SET | CA_REMOVE | CA_INVITE)
#define CA_SUCCESSOR     (CA_SOP | CA_RECOVER)
#define CA_FOUNDER       (CA_SUCCESSOR | CA_FLAGS)

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
	char 	 subject[30];
	char 	 text[MEMOLEN];
	time_t	 sent;
	uint32_t status;
	list_t	 metadata;
};

/* memo status flags */
#define MEMO_NEW           0x00000000
#define MEMO_READ          0x00000001

#endif
