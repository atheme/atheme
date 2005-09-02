/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol handlers, both generic and the actual declarations themselves.
 * Declare NOTYET to use the function pointer voodoo.
 *
 * $Id: phandler.h 1692 2005-08-12 19:34:45Z nenolod $
 */

#ifndef PHANDLER_H
#define PHANDLER_H

struct ircd_ {
	char *ircdname;
	char *tldprefix;
	boolean_t uses_uid;
	boolean_t uses_rcommand;
	boolean_t uses_owner;
	boolean_t uses_protect;
	boolean_t uses_halfops;
	boolean_t uses_p10;		/* Parser hackhack. */
	boolean_t uses_vhost;		/* Do we use vHosts? */
	uint32_t owner_mode;
	uint32_t protect_mode;
	uint32_t halfops_mode;
	char *owner_mchar;
	char *protect_mchar;
	char *halfops_mchar;
};

typedef struct ircd_ ircd_t;

extern uint8_t (*server_login)(void);
extern user_t *(*introduce_nick)(char *nick, char *user, char *host, char *real, char *modes);
extern void (*wallops)(char *fmt, ...);
extern void (*join)(char *chan, char *nick);
extern void (*kick)(char *from, char *channel, char *to, char *reason);
extern void (*msg)(char *from, char *target, char *fmt, ...);
extern void (*notice)(char *from, char *target, char *fmt, ...);
extern void (*numeric_sts)(char *from, int numeric, char *target, char *fmt, ...);
extern void (*skill)(char *from, char *nick, char *fmt, ...);
extern void (*part)(char *chan, char *nick);
extern void (*kline_sts)(char *server, char *user, char *host, long duration, char *reason);
extern void (*unkline_sts)(char *server, char *user, char *host);
extern void (*topic_sts)(char *channel, char *setter, char *topic);
extern void (*mode_sts)(char *sender, char *target, char *modes);
extern void (*ping_sts)(void);
extern void (*quit_sts)(user_t *u, char *reason);
extern void (*ircd_on_login)(char *origin, char *user, char *wantedhost);
extern void (*ircd_on_logout)(char *origin, char *user, char *wantedhost);
extern void (*jupe)(char *server, char *reason);

extern uint8_t generic_server_login(void);
extern user_t *generic_introduce_nick(char *nick, char *ser, char *host, char *real, char *modes);
extern void generic_wallops(char *fmt, ...);
extern void generic_join(char *chan, char *nick);
extern void generic_kick(char *from, char *channel, char *to, char *reason);
extern void generic_msg(char *from, char *target, char *fmt, ...);
extern void generic_notice(char *from, char *target, char *fmt, ...);
extern void generic_numeric_sts(char *from, int numeric, char *target, char *fmt, ...);
extern void generic_skill(char *from, char *nick, char *fmt, ...);
extern void generic_part(char *chan, char *nick);
extern void generic_kline_sts(char *server, char *user, char *host, long duration, char *reason);
extern void generic_unkline_sts(char *server, char *user, char *host);
extern void generic_topic_sts(char *channel, char *setter, char *topic);
extern void generic_mode_sts(char *sender, char *target, char *modes);
extern void generic_ping_sts(void);
extern void generic_quit_sts(user_t *u, char *reason);
extern void generic_on_login(char *origin, char *user, char *wantedhost);
extern void generic_on_logout(char *origin, char *user, char *wantedhost);
extern void generic_jupe(char *server, char *reason);

extern struct cmode_ *mode_list;
extern struct cmode_ *ignore_mode_list;
extern struct cmode_ *status_mode_list;
extern struct cmode_ *prefix_mode_list;

extern ircd_t *ircd;

/* This stuff is for databases, but it's the same sort of concept. */
extern void (*db_save)(void *arg);
extern void (*db_load)(void);

#endif

