/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * Global data
 */

#ifndef ATHEME_INC_GLOBAL_H
#define ATHEME_INC_GLOBAL_H 1

#include <atheme/stdheaders.h>

/* me, a struct containing basic configuration options and some dynamic
 * information about our uplink and program state */
struct me
{
	char *          name;                   // server's name on IRC
	char *          desc;                   // server's description
	char *          actual;                 // the reported name of the uplink
	char *          vhost;                  // IP we bind outgoing stuff to
	unsigned int    recontime;              // time between reconnection attempts
	char *          netname;                // IRC network name
	char *          hidehostsuffix;         // host suffix for P10 +x etc
	char *          adminname;              // SRA's name (for ADMIN)
	char *          adminemail;             // SRA's email (for ADMIN)
	char *          mta;                    // path to mta program
	char *          numeric;                // server numeric
	unsigned int    mdlimit;                // metadata entry limit
	time_t          start;                  // starting time
	struct server * me;                     // pointer to our server struct
	bool            connected;              // are we connected?
	bool            bursting;               // are we bursting?
	bool            recvsvr;                // received server peer
	unsigned int    maxcertfp;              // maximum fingerprints in certfp list
	unsigned int    maxlogins;              // maximum logins per username
	unsigned int    maxusers;               // maximum usernames from one email
	unsigned int    maxmemos;
	unsigned int    auth;                   // registration auth type
	unsigned int    emaillimit;             // maximum number of emails sent
	unsigned int    emailtime;              // ... in this amount of time
	unsigned long   kline_id;               // unique ID for AKILLs
	unsigned long   xline_id;               // unique ID for AKILLs
	unsigned long   qline_id;               // unique ID for AKILLs
	time_t          uplinkpong;             // when the uplink last sent a PONG
	char *          execname;               // executable name
	char *          language_name;          // language file name
	char *          language_translator;    // translator name
	char *          register_email;         // from address on e-mails
	bool            hidden;                 // whether or not we should hide ourselves in /links (if the ircd supports it)
};

extern struct me me;

/* values for me.auth */
#define AUTH_NONE  0
#define AUTH_EMAIL 1

/* config_options, a struct containing other global configuration options */
struct ConfOption
{
	char *          helpchan;               // official help channel
	char *          helpurl;                // official help URL
	unsigned int    flood_msgs;             // messages determining flood
	unsigned int    flood_time;             // time determining flood
	unsigned int    ratelimit_uses;         // uses of a ratelimited command
	unsigned int    ratelimit_period;       // period in which ratelimit_uses are done
	unsigned int    kline_time;             // default expire for klines
	unsigned int    vhost_change;           // days in which a user must wait between vhost changes
	unsigned int    clone_time;             // default expire for clone exemptions
	unsigned int    commit_interval;        // interval between commits
	bool            db_save_blocking;       // whether to always use a blocking database commit
	bool            silent;                 // stop sending WALLOPS?
	bool            join_chans;             // join registered channels?
	bool            leave_chans;            // leave channels when empty?
	bool            secure;                 // require /msg <service>@host?
	bool            kline_with_ident;       // kline ident@host instead of *@host on automated bans?
	bool            kline_verified_ident;   // Don't kline ident@host if first char of ident is ~ ?
	unsigned int    defuflags;              // default username flags
	unsigned int    defcflags;              // default channel flags
	bool            raw;                    // enable raw/inject?
	char *          global;                 // nick for global noticer
	char *          languagefile;           // path to language file (if any)
	bool            verbose_wallops;        // verbose wallops? :)
	char *          operstring;             // "is an IRC Operator"
	char *          servicestring;          // "is a Network Service"
	unsigned int    default_clone_allowed;  // default clone kill
	unsigned int    default_clone_warn;     // default clone warn
	bool            clone_increase;         // If the clone limit will increase based on # of identified clones
	unsigned int    uplink_sendq_limit;
	char *          language;               // default language
	mowgli_list_t   exempts;                // List of masks never to automatically kline
	bool            allow_taint;            // allow tainted operation
	unsigned int    immune_level;           // what flag is required for kick immunity
	bool            show_entity_id;         // do not require user:auspex to see entity IDs
	bool            load_database_mdeps;    // for core module deps listed in DB, whether to load them or abort
	bool            hide_opers;             // whether or not to hide RPL_WHOISOPERATOR from remote whois
	bool            masks_through_vhost;    // whether masks match the host/IP behind a vhost
	unsigned int    default_pass_length;    // the default length for services-generated passwords (resetpass,
	                                        // sendpass, return)
};

extern struct ConfOption config_options;

/* keep track of how many of what we have */
struct cnt
{
	unsigned int    event;
	unsigned int    soper;
	unsigned int    svsignore;
	unsigned int    tld;
	unsigned int    kline;
	unsigned int    xline;
	unsigned int    qline;
	unsigned int    server;
	unsigned int    user;
	unsigned int    chan;
	unsigned int    chanuser;
	unsigned int    myuser;
	unsigned int    mynick;
	unsigned int    mychan;
	unsigned int    chanacs;
	unsigned int    node;
	unsigned int    bin;
	unsigned int    bout;
	unsigned int    uplink;
	unsigned int    operclass;
	unsigned int    myuser_access;
	unsigned int    myuser_name;
};

extern struct cnt cnt;

struct claro_state
{
	unsigned int    node;
	unsigned int    event;
	time_t          currtime;
};

extern struct claro_state claro_state;

#define CURRTIME claro_state.currtime

/* run flags */
extern int runflags;

#define RF_LIVE         0x00000001U      /* don't fork  */
#define RF_SHUTDOWN     0x00000002U      /* shut down   */
#define RF_STARTING     0x00000004U      /* starting up */
#define RF_RESTART      0x00000008U      /* restart     */
#define RF_REHASHING    0x00000010U      /* rehashing   */

/* node.c */
void init_nodes(void);
/* The following currently only do uplinks -- jilles */
void mark_all_illegal(void);
void unmark_all_illegal(void);
void remove_illegals(void);

/* atheme.c */
void db_save_periodic(void *);
extern mowgli_eventloop_t *base_eventloop;
extern mowgli_eventloop_timer_t *commit_interval_timer;
extern bool cold_start;
extern bool readonly;
extern bool offline_mode;
extern bool permissive_mode;
extern bool database_create;
extern char *config_file;
extern char *datadir;

/* conf.c */
const char *get_conf_opts(void);

/* version.c */
extern const char *creation;
extern const char *platform;
extern const char *version;
extern const char *revision;
extern const char *osinfo;
extern const char *infotext[];

/* signal.c */
void check_signals(void);
void childproc_add(pid_t pid, const char *desc, void (*cb)(pid_t pid, int status, void *data), void *data);
void childproc_delete_all(void (*cb)(pid_t pid, int status, void *data));

#endif /* !ATHEME_INC_GLOBAL_H */
