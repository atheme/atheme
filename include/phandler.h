/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol handlers, both generic and the actual declarations themselves.
 * Declare NOTYET to use the function pointer voodoo.
 *
 * $Id: phandler.h 3979 2005-11-26 01:35:34Z jilles $
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
	uint32_t oper_only_modes;
	uint32_t owner_mode;
	uint32_t protect_mode;
	uint32_t halfops_mode;
	char *owner_mchar;
	char *protect_mchar;
	char *halfops_mchar;
	uint8_t type;
};

typedef struct ircd_ ircd_t;

E uint8_t (*server_login)(void);
E void (*introduce_nick)(char *nick, char *user, char *host, char *real, char *uid);
E void (*wallops)(char *fmt, ...);
E void (*join_sts)(channel_t *c, user_t *u, boolean_t isnew, char *modes);
E void (*kick)(char *from, char *channel, char *to, char *reason);
E void (*msg)(char *from, char *target, char *fmt, ...);
E void (*notice)(char *from, char *target, char *fmt, ...);
E void (*numeric_sts)(char *from, int numeric, char *target, char *fmt, ...);
E void (*skill)(char *from, char *nick, char *fmt, ...);
E void (*part)(char *chan, char *nick);
E void (*kline_sts)(char *server, char *user, char *host, long duration, char *reason);
E void (*unkline_sts)(char *server, char *user, char *host);
E void (*topic_sts)(char *channel, char *setter, time_t ts, char *topic);
E void (*mode_sts)(char *sender, char *target, char *modes);
E void (*ping_sts)(void);
E void (*quit_sts)(user_t *u, char *reason);
E void (*ircd_on_login)(char *origin, char *user, char *wantedhost);
E void (*ircd_on_logout)(char *origin, char *user, char *wantedhost);
E void (*jupe)(char *server, char *reason);
E void (*sethost_sts)(char *source, char *target, char *host);
E void (*fnc_sts)(user_t *source, user_t *u, char *newnick, int type);

E uint8_t generic_server_login(void);
E void generic_introduce_nick(char *nick, char *ser, char *host, char *real, char *uid);
E void generic_wallops(char *fmt, ...);
E void generic_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes);
E void generic_kick(char *from, char *channel, char *to, char *reason);
E void generic_msg(char *from, char *target, char *fmt, ...);
E void generic_notice(char *from, char *target, char *fmt, ...);
E void generic_numeric_sts(char *from, int numeric, char *target, char *fmt, ...);
E void generic_skill(char *from, char *nick, char *fmt, ...);
E void generic_part(char *chan, char *nick);
E void generic_kline_sts(char *server, char *user, char *host, long duration, char *reason);
E void generic_unkline_sts(char *server, char *user, char *host);
E void generic_topic_sts(char *channel, char *setter, time_t ts, char *topic);
E void generic_mode_sts(char *sender, char *target, char *modes);
E void generic_ping_sts(void);
E void generic_quit_sts(user_t *u, char *reason);
E void generic_on_login(char *origin, char *user, char *wantedhost);
E void generic_on_logout(char *origin, char *user, char *wantedhost);
E void generic_jupe(char *server, char *reason);
E void generic_sethost_sts(char *source, char *target, char *host);
E void generic_fnc_sts(user_t *source, user_t *u, char *newnick, int type);

E struct cmode_ *mode_list;
E struct cmode_ *ignore_mode_list;
E struct cmode_ *status_mode_list;
E struct cmode_ *prefix_mode_list;

E ircd_t *ircd;

/* This stuff is for databases, but it's the same sort of concept. */
E void (*db_save)(void *arg);
E void (*db_load)(void);

#endif

