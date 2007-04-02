/*
 * Copyright (C) 2003-2004 E. Will et al.
 * Copyright (C) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Global data
 *
 * $Id: global.h 8027 2007-04-02 10:47:18Z nenolod $
 */

#ifndef _GLOBAL_H
#define _GLOBAL_H

/* me, a struct containing basic configuration options and some dynamic
 * information about our uplink and program state */
typedef struct me me_t;

/* S T R U C T U R E S */
struct me
{
  char *name;                   /* server's name on IRC               */
  char *desc;                   /* server's description               */
  char *actual;                 /* the reported name of the uplink    */
  char *vhost;                  /* IP we bind outgoing stuff to       */
  unsigned int recontime;           /* time between reconnection attempts */
  unsigned int restarttime;         /* time before restarting             */
  char *netname;                /* IRC network name                   */
  char *hidehostsuffix;         /* host suffix for P10 +x etc         */
  char *adminname;              /* SRA's name (for ADMIN)             */
  char *adminemail;             /* SRA's email (for ADMIN             */
  char *mta;                    /* path to mta program                */
  char *numeric;		/* server numeric		      */

  unsigned int loglevel;            /* logging level                      */
  unsigned int maxfd;               /* how many fds do we have?           */
  unsigned int mdlimit;		/* metadata entry limit		      */
  time_t start;                 /* starting time                      */
  server_t *me;                 /* pointer to our server struct       */
  boolean_t connected;          /* are we connected?                  */
  boolean_t bursting;           /* are we bursting?                   */
  boolean_t recvsvr;		/* received server peer               */

  unsigned int maxlogins;           /* maximum logins per username        */
  unsigned int maxusers;            /* maximum usernames from one email   */
  unsigned int maxnicks;            /* maximum nicks from one username    */
  unsigned int maxchans;            /* maximum chans from one username    */
  unsigned int auth;                /* registration auth type             */
  unsigned int emaillimit;          /* maximum number of emails sent      */
  unsigned int emailtime;           /* ... in this amount of time         */

  time_t uplinkpong;            /* when the uplink last sent a PONG   */

  char *execname;		/* executable name                    */

  char *language_name;		/* language file name		      */
  char *language_translator;	/* translator name		      */
};

E me_t me;

/* values for me.auth */
#define AUTH_NONE  0
#define AUTH_EMAIL 1

/* config_options, a struct containing other global configuration options */
struct ConfOption
{
  char *chan;                   /* channel we join/msg        */

  unsigned int flood_msgs;          /* messages determining flood */
  unsigned int flood_time;          /* time determining flood     */
  unsigned int kline_time;          /* default expire for klines  */
  unsigned int commit_interval;     /* interval between commits   */
  int expire;               /* time before registrations expire */

  boolean_t silent;             /* stop sending WALLOPS?      */
  boolean_t join_chans;         /* join registered channels?  */
  boolean_t leave_chans;        /* leave channels when empty? */
  boolean_t secure;             /* require /msg <service>@host? */

  unsigned int defuflags;           /* default username flags     */
  unsigned int defcflags;           /* default channel flags      */

  boolean_t raw;                /* enable raw/inject?         */

  char *global;                 /* nick for global noticer    */
  char *languagefile;		/* path to language file (if any) */

  boolean_t verbose_wallops;	/* verbose wallops? :)        */
  boolean_t use_privmsg;        /* use privmsg instead of notice */
} config_options;

struct Database
{
  char *user;
  char *pass;
  char *database;
  char *host;
  unsigned int port;
} database_options;

/* keep track of how many of what we have */
struct cnt
{
  unsigned int event;
  unsigned int soper;
  unsigned int svsignore;
  unsigned int tld;
  unsigned int kline;
  unsigned int server;
  unsigned int user;
  unsigned int chan;
  unsigned int chanuser;
  unsigned int myuser;
  unsigned int mynick;
  unsigned int mychan;
  unsigned int chanacs;
  unsigned int node;
  unsigned int bin;
  unsigned int bout;
  unsigned int uplink;
  unsigned int operclass;
  unsigned int myuser_access;
};

E struct cnt cnt;

typedef struct claro_state_ {
	unsigned int node;
	unsigned int event;
	time_t currtime;
	unsigned int maxfd;
} claro_state_t;

E claro_state_t claro_state;

#define CURRTIME claro_state.currtime

/* run flags */
E int runflags;

#define RF_LIVE         0x00000001      /* don't fork  */
#define RF_SHUTDOWN     0x00000002      /* shut down   */
#define RF_STARTING     0x00000004      /* starting up */
#define RF_RESTART      0x00000008      /* restart     */
#define RF_REHASHING    0x00000010      /* rehashing   */

/* conf.c */
E boolean_t conf_parse(char *);
E void conf_init(void);
E boolean_t conf_rehash(void);
E boolean_t conf_check(void);

/* confp.c */
E void config_free(CONFIGFILE *cfptr);
E CONFIGFILE *config_load(char *filename);
E CONFIGENTRY *config_find(CONFIGENTRY *ceptr, char *name);

/* node.c */
E void init_nodes(void);
/* The following currently only do uplinks -- jilles */
E void mark_all_illegal(void);
E void unmark_all_illegal(void);
E void remove_illegals(void);

/* atheme.c */
E boolean_t cold_start;
E char *config_file;

/* version.c */
E const char *generation;
E const char *creation;
E const char *platform;
E const char *version;
E const char *revision;
E const char *osinfo;
E const char *infotext[];

/* signal.c */
E void sighandler(int signum);
E void check_signals(void);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
