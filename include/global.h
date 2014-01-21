/*
 * Copyright (C) 2003-2004 E. Will et al.
 * Copyright (C) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Global data
 *
 */

#ifndef ATHEME_GLOBAL_H
#define ATHEME_GLOBAL_H

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

  int maxfd;                    /* how many fds do we have?           */
  unsigned int mdlimit;		/* metadata entry limit		      */
  time_t start;                 /* starting time                      */
  server_t *me;                 /* pointer to our server struct       */
  bool connected;          /* are we connected?                  */
  bool bursting;           /* are we bursting?                   */
  bool recvsvr;		/* received server peer               */

  unsigned int maxlogins;           /* maximum logins per username        */
  unsigned int maxusers;            /* maximum usernames from one email   */
  unsigned int maxmemos;
  unsigned int auth;                /* registration auth type             */
  unsigned int emaillimit;          /* maximum number of emails sent      */
  unsigned int emailtime;           /* ... in this amount of time         */

  unsigned long kline_id;	/* unique ID for AKILLs			*/
  unsigned long xline_id;	/* unique ID for AKILLs			*/
  unsigned long qline_id;	/* unique ID for AKILLs			*/

  time_t uplinkpong;            /* when the uplink last sent a PONG   */

  char *execname;		/* executable name                    */

  char *language_name;		/* language file name		      */
  char *language_translator;	/* translator name		      */

  char *register_email;         /* from address on e-mails            */

  bool hidden;			/* whether or not we should hide ourselves in /links (if the ircd supports it) */
};

E me_t me;

/* values for me.auth */
#define AUTH_NONE  0
#define AUTH_EMAIL 1

/* config_options, a struct containing other global configuration options */
struct ConfOption
{
  char *helpchan;		/* official help channel    */
  char *helpurl;		/* official help URL	    */

  unsigned int flood_msgs;          /* messages determining flood */
  unsigned int flood_time;          /* time determining flood     */
  unsigned int ratelimit_uses;	    /* uses of a ratelimited command */
  unsigned int ratelimit_period;    /* period in which ratelimit_uses are done */
  unsigned int kline_time;          /* default expire for klines  */
  unsigned int clone_time;          /* default expire for clone exemptions */
  unsigned int commit_interval;     /* interval between commits   */

  bool silent;               /* stop sending WALLOPS?      */
  bool join_chans;           /* join registered channels?  */
  bool leave_chans;          /* leave channels when empty? */
  bool secure;               /* require /msg <service>@host? */
  bool kline_with_ident;     /* kline ident@host instead of *@host on automated bans? */
  bool kline_verified_ident; /* Don't kline ident@host if first char of ident is ~ ? */

  unsigned int defuflags;           /* default username flags     */
  unsigned int defcflags;           /* default channel flags      */

  bool raw;                /* enable raw/inject?         */

  char *global;                 /* nick for global noticer    */
  char *languagefile;		/* path to language file (if any) */

  bool verbose_wallops;	/* verbose wallops? :)        */

  unsigned int default_clone_allowed;  /* default clone kill */
  unsigned int default_clone_warn;  /* default clone warn */
  bool clone_increase;  /* If the clone limit will increase based on # of identified clones */

  unsigned int uplink_sendq_limit;

  char *language;		/* default language */

  mowgli_list_t exempts;		/* List of masks never to automatically kline */

  bool allow_taint;		/* allow tainted operation */

  unsigned int immune_level;	/* what flag is required for kick immunity */
};

E struct ConfOption config_options;

/* keep track of how many of what we have */
struct cnt
{
  unsigned int event;
  unsigned int soper;
  unsigned int svsignore;
  unsigned int tld;
  unsigned int kline;
  unsigned int xline;
  unsigned int qline;
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
  unsigned int myuser_name;
};

E struct cnt cnt;

typedef struct claro_state_ {
	unsigned int node;
	unsigned int event;
	time_t currtime;
	int maxfd;
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

/* node.c */
E void init_nodes(void);
/* The following currently only do uplinks -- jilles */
E void mark_all_illegal(void);
E void unmark_all_illegal(void);
E void remove_illegals(void);

/* atheme.c */
E mowgli_eventloop_t *base_eventloop;
E bool cold_start;
E bool readonly;
E bool offline_mode;
E bool permissive_mode;
E char *config_file;
E char *datadir;

/* conf.c */
E const char *get_conf_opts(void);

/* version.c */
E const char *creation;
E const char *platform;
E const char *version;
E const char *revision;
E const char *osinfo;
E const char *infotext[];

/* signal.c */
E void check_signals(void);
E void childproc_add(pid_t pid, const char *desc, void (*cb)(pid_t pid, int status, void *data), void *data);
E void childproc_delete_all(void (*cb)(pid_t pid, int status, void *data));

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
