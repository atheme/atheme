/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This is the main header file, usually the only one #include'd
 *
 * $Id: atheme.h 3011 2005-10-19 05:11:16Z nenolod $
 */

#ifndef ATHEME_H
#define ATHEME_H

/* *INDENT-OFF* */

#include <org.atheme.claro.base>

#include "common.h"
#include "servers.h"
#include "channels.h"
#include "module.h"
#include "commandtree.h"
#include "pmodule.h"
#include "serno.h"
#include "crypto.h"
#include "culture.h"
#include "xmlrpc.h"

#ifndef timersub
#define timersub(tvp, uvp, vvp)                                         \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;       \
                if ((vvp)->tv_usec < 0) {                               \
                        (vvp)->tv_sec--;                                \
                        (vvp)->tv_usec += 1000000;                      \
                }                                                       \
        } while (0)
#endif

/* hashing macros */
#define SHASH(server) shash(server)
#define UHASH(nick) shash(nick)
#define CHASH(chan) shash(chan)
#define MUHASH(myuser) shash(myuser)
#define MCHASH(mychan) shash(mychan)

/* other stuff. */
#define CLIENT_NAME(user)	((user)->uid[0] ? (user)->uid : (user)->nick)
#define SERVER_NAME(serv)	((serv)->sid[0] ? (serv)->sid : (serv)->name)
#define ME			(ircd->uses_uid ? curr_uplink->numeric : me.name)

typedef struct {
        channel_t *c;
        char *msg;
} hook_cmessage_data_t;

/* T Y P E D E F S */
typedef struct sendq datastream_t;

typedef struct tld_ tld_t;
typedef struct kline_ kline_t;

typedef struct me me_t;

/* S T R U C T U R E S */
struct me
{
  char *name;                   /* server's name on IRC               */
  char *desc;                   /* server's description               */
  char *uplink;                 /* the server we connect to           */
  char *actual;                 /* the reported name of the uplink    */
  uint16_t port;                /* port we connect to our uplink on   */
  char *vhost;                  /* IP we bind outgoing stuff to       */
  uint16_t recontime;           /* time between reconnection attempts */
  uint16_t restarttime;         /* time before restarting             */
  char *netname;                /* IRC network name                   */
  char *adminname;              /* SRA's name (for ADMIN)             */
  char *adminemail;             /* SRA's email (for ADMIN             */
  char *mta;                    /* path to mta program           */
  char *numeric;		/* server numeric		      */

  uint8_t loglevel;             /* logging level                      */
  uint32_t maxfd;               /* how many fds do we have?           */
  uint32_t mdlimit;		/* metadata entry limit		      */
  time_t start;                 /* starting time                      */
  server_t *me;                 /* pointer to our server struct       */
  boolean_t connected;          /* are we connected?                  */
  boolean_t bursting;           /* are we bursting?                   */
  boolean_t recvsvr;		/* used by P10: recieved server peer  */

  uint16_t maxlogins;           /* maximum logins per username        */
  uint16_t maxusers;            /* maximum usernames from one email   */
  uint16_t maxchans;            /* maximum chans from one username    */
  uint8_t auth;                 /* registration auth type             */

  time_t uplinkpong;            /* when the uplink last sent a PONG   */

  char *execname;		/* executable name                    */
};

E me_t me;

#define AUTH_NONE  0
#define AUTH_EMAIL 1

struct ConfOption
{
  char *chan;                   /* channel we join/msg        */

  uint16_t flood_msgs;          /* messages determining flood */
  uint16_t flood_time;          /* time determining flood     */
  uint32_t kline_time;          /* default expire for klines  */
  uint16_t commit_interval;     /* interval between commits   */
  int32_t expire;               /* time before registrations expire */

  boolean_t silent;             /* stop sending WALLOPS?      */
  boolean_t join_chans;         /* join registered channels?  */
  boolean_t leave_chans;        /* leave channels when empty? */
  boolean_t secure;             /* require /msg <service>@host? */

  uint16_t defuflags;           /* default username flags     */
  uint16_t defcflags;           /* default channel flags      */

  boolean_t raw;                /* enable raw/inject?         */

  char *global;                 /* nick for global noticer    */
} config_options;

struct Database
{
  char *user;
  char *pass;
  char *database;
  char *host;
  uint32_t port;
} database_options;

/* keep track of how many of what we have */
typedef struct cnt cnt_t;

struct cnt
{
  uint32_t event;
  uint32_t sra;
  uint32_t tld;
  uint32_t kline;
  uint32_t server;
  uint32_t user;
  uint32_t chan;
  uint32_t chanuser;
  uint32_t myuser;
  uint32_t mychan;
  uint32_t chanacs;
  uint32_t node;
  uint32_t bin;
  uint32_t bout;
  uint32_t uplink;
};

E cnt_t cnt;

#define MTYPE_NUL 0
#define MTYPE_ADD 1
#define MTYPE_DEL 2

struct cmode_
{
        char mode;
        uint32_t value;
};

/* tld list struct */
struct tld_ {
  char *name;
};

/* kline list struct */
struct kline_ {
  char *user;
  char *host;
  char *reason;
  char *setby;

  uint16_t number;
  long duration;
  time_t settime;
  time_t expires;
};

/* global list struct */
struct global_ {
  char *text;
};

/* sendq struct */
struct sendq {
  char *buf;
  int len;
  int pos;
};

/* database versions */
#define DB_SHRIKE	1
#define DB_ATHEME	2

/* struct for irc message hash table */
struct message_
{
  const char *name;
  void (*func) (char *origin, uint8_t parc, char *parv[]);
};

/* struct for command hash table */
struct command_
{
  const char *name;
  uint8_t access;
  void (*func) (char *nick);
};

/* struct for set command hash table */
struct set_command_
{
  const char *name;
  uint8_t access;
  void (*func) (char *origin, char *name, char *params);
};

typedef struct help_command_ helpentry_t;

/* struct for help command hash table */
struct help_command_
{
  char *name;
  uint8_t access;
  char *file;
  void (*func) (char *origin);
};

#define AC_NONE  0
#define AC_IRCOP 1
#define AC_SRA   2

/* run flags */
int runflags;

#define RF_LIVE         0x00000001      /* don't fork  */
#define RF_SHUTDOWN     0x00000002      /* shut down   */
#define RF_STARTING     0x00000004      /* starting up */
#define RF_RESTART      0x00000008      /* restart     */
#define RF_REHASHING    0x00000010      /* rehashing   */

/* log levels */
#define LG_NONE         0x00000001      /* don't log                */
#define LG_INFO         0x00000002      /* log general info         */
#define LG_ERROR        0x00000004      /* log real important stuff */
#define LG_IOERROR	0x00000008	/* log I/O errors. */
#define LG_DEBUG        0x00000010      /* log debugging stuff      */

/* bursting timer */
#if HAVE_GETTIMEOFDAY
struct timeval burstime;
#endif

/* *INDENT-OFF* */

/* down here so stuff it uses in here works */
#include "account.h"
#include "confparse.h"
#include "flags.h"
#include "extern.h"
#include "metadata.h"
#include "phandler.h"
#include "servtree.h"
#include "services.h"
#include "users.h"
#include "authcookie.h"

/* *INDENT-ON* */

#ifdef _WIN32

/* Windows + Module -> needs these to be declared before using them */
#ifdef I_AM_A_MODULE
void _modinit(module_t *m);
void _moddeinit(void);
#endif

/* Windows has an extremely stupid gethostbyname() function. Oof! */
#define gethostbyname(a) gethostbyname_layer(a)

#endif

#endif /* ATHEME_H */
