/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * Data structures related to services psuedo-clients.
 */

#ifndef ATHEME_INC_SERVICES_H
#define ATHEME_INC_SERVICES_H 1

#include <atheme/attributes.h>
#include <atheme/common.h>
#include <atheme/constants.h>
#include <atheme/stdheaders.h>
#include <atheme/structures.h>

/* The nick/user/host/real strings in these structs simply point
 * to their counterparts in the struct service, and will probably be removed
 * at some point.
 */
/* core services */
struct chansvs
{
	char *          nick;               // the IRC client's nickname
	char *          user;               // the IRC client's username
	char *          host;               // the IRC client's hostname
	char *          real;               // the IRC client's realname
	bool            fantasy;            // enable fantasy commands
	char *          trigger;            // trigger, e.g. !, ` or .
	bool            changets;           // use TS to better deop people
	struct service *me;                 // our struct user
	unsigned int    expiry;             // expiry time
	unsigned int    akick_time;         // default akick duration
	unsigned int    maxchans;           // max channels one can register
	unsigned int    maxchanacs;         // max entries in chanacs list
	unsigned int    maxfounders;        // max founders per channel
	char *          founder_flags;      // default founder flags for new channels
	char *          deftemplates;       // default templates
	bool            hide_xop;           // hide XOP templates
	bool            hide_pubacl_akicks; // hide AKICKs only visible via PUBACL
};

/* authentication services */
struct nicksvs
{
	bool            spam;
	bool            no_nick_ownership;
	char *          nick;
	char *          user;
	char *          host;
	char *          real;
	struct service *me;
	unsigned int    maxnicks;       // max nicknames one can group
	unsigned int    expiry;         // expiry time
	unsigned int    enforce_expiry; // expiry time
	unsigned int    enforce_delay;  // delay for nickname enforce
	char *          enforce_prefix; // prefix for enforcement
	mowgli_list_t   emailexempts;   // emails exempt from maxusers checks
};

/* atheme.c */
extern struct chansvs chansvs;
extern struct nicksvs nicksvs;

/* services.c */
extern int authservice_loaded;
extern int use_myuser_access;
extern int use_svsignore;
extern int use_privmsg;
extern int use_account_private;
extern int use_channel_private;
extern int use_limitflags;

unsigned int ban(struct user *source, struct channel *chan, struct user *target);
unsigned int remove_banlike(struct user *source, struct channel *chan, int type, struct user *target);
unsigned int remove_ban_exceptions(struct user *source, struct channel *chan, struct user *target);

bool try_kick_real(struct user *source, struct channel *chan, struct user *target, const char *reason);
extern bool (*try_kick)(struct user *source, struct channel *chan, struct user *target, const char *reason);

void kill_user(struct user *source, struct user *victim, const char *fmt, ...) ATHEME_FATTR_PRINTF(3, 4);
void introduce_enforcer(const char *nick);
void join(const char *chan, const char *nick);
void joinall(const char *name);
void part(const char *chan, const char *nick);
void partall(const char *name);
void myuser_login(struct service *svs, struct user *u, struct myuser *mu, bool sendaccount);
void verbose(const struct mychan *mychan, const char *fmt, ...) ATHEME_FATTR_PRINTF(2, 3);
extern void (*notice)(const char *from, const char *target, const char *fmt, ...) ATHEME_FATTR_PRINTF(3, 4);
void change_notify(const char *from, struct user *to, const char *message, ...) ATHEME_FATTR_PRINTF(3, 4);
bool bad_password(struct sourceinfo *si, struct myuser *mu);
bool ircd_logout_or_kill(struct user *u, const char *login);

struct sourceinfo *sourceinfo_create(void);
void command_fail(struct sourceinfo *si, enum cmd_faultcode code, const char *fmt, ...) ATHEME_FATTR_PRINTF(3, 4);
void command_success_nodata(struct sourceinfo *si, const char *fmt, ...) ATHEME_FATTR_PRINTF(2, 3);
void command_success_string(struct sourceinfo *si, const char *result, const char *fmt, ...) ATHEME_FATTR_PRINTF(3, 4);
void command_success_table(struct sourceinfo *si, struct atheme_table *table);
const char *get_source_name(struct sourceinfo *si);
const char *get_source_mask(struct sourceinfo *si);
const char *get_oper_name(struct sourceinfo *si);
const char *get_storage_oper_name(struct sourceinfo *si);
const char *get_source_security_label(struct sourceinfo *si);

void wallops(const char *, ...) ATHEME_FATTR_PRINTF(1, 2);
void verbose_wallops(const char *, ...) ATHEME_FATTR_PRINTF(1, 2);
bool check_vhost_validity(struct sourceinfo *si, const char *host);

/* ptasks.c */
void handle_topic(struct channel *, const char *, time_t, const char *);
int floodcheck(struct user *, struct user *);
void command_add_flood(struct sourceinfo *si, unsigned int amount);

/* ctcp-common.c */
void common_ctcp_init(void);
unsigned int handle_ctcp_common(struct sourceinfo *si, char *, char *);

#ifdef HAVE_LIBQRENCODE
/* qrcode.c */
void command_success_qrcode(struct sourceinfo *si, const char *data);
#endif /* HAVE_LIBQRENCODE */

#endif /* !ATHEME_INC_SERVICES_H */
