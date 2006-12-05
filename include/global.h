/*
 * Copyright (C) 2003-2004 E. Will et al.
 * Copyright (C) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Global data
 *
 * $Id: global.h 7317 2006-12-05 00:14:26Z jilles $
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
  char *uplink;                 /* the server we connect to           */
  char *actual;                 /* the reported name of the uplink    */
  char *vhost;                  /* IP we bind outgoing stuff to       */
  uint16_t recontime;           /* time between reconnection attempts */
  uint16_t restarttime;         /* time before restarting             */
  char *netname;                /* IRC network name                   */
  char *hidehostsuffix;         /* host suffix for P10 +x etc         */
  char *adminname;              /* SRA's name (for ADMIN)             */
  char *adminemail;             /* SRA's email (for ADMIN             */
  char *mta;                    /* path to mta program                */
  char *numeric;		/* server numeric		      */

  uint32_t loglevel;            /* logging level                      */
  uint32_t maxfd;               /* how many fds do we have?           */
  uint32_t mdlimit;		/* metadata entry limit		      */
  time_t start;                 /* starting time                      */
  server_t *me;                 /* pointer to our server struct       */
  boolean_t connected;          /* are we connected?                  */
  boolean_t bursting;           /* are we bursting?                   */
  boolean_t recvsvr;		/* received server peer               */

  uint16_t maxlogins;           /* maximum logins per username        */
  uint16_t maxusers;            /* maximum usernames from one email   */
  uint16_t maxchans;            /* maximum chans from one username    */
  uint8_t auth;                 /* registration auth type             */
  uint16_t emaillimit;          /* maximum number of emails sent      */
  uint16_t emailtime;           /* ... in this amount of time         */

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
  uint32_t port;
} database_options;

/* keep track of how many of what we have */
struct cnt
{
  uint32_t event;
  uint32_t soper;
  uint32_t svsignore;
  uint32_t tld;
  uint32_t kline;
  uint32_t server;
  uint32_t user;
  uint32_t chan;
  uint32_t chanuser;
  uint32_t myuser;
  uint32_t mynick;
  uint32_t mychan;
  uint32_t chanacs;
  uint32_t node;
  uint32_t bin;
  uint32_t bout;
  uint32_t uplink;
  uint32_t operclass;
  uint32_t myuser_access;
};

E struct cnt cnt;

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
