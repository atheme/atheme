/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol handlers, both generic and the actual declarations themselves.
 *
 * $Id: phandler.h 5628 2006-07-01 23:38:42Z jilles $
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
	uint32_t perm_mode;		/* Modes to not disappear when empty */
	char *ban_like_modes;		/* e.g. "beI" */
	char except_mchar;
	char invex_mchar;
};

typedef struct ircd_ ircd_t;

/* server login, usually sends PASS, CAPAB, SERVER and SVINFO
 * you can still change ircd->uses_uid at this point
 * set me.bursting = TRUE
 * return 1 if sts() failed (by returning 1), otherwise 0 */
E uint8_t (*server_login)(void);
/* introduce a client on the services server */
E void (*introduce_nick)(char *nick, char *user, char *host, char *real, char *uid);
/* send an invite for a given user to a channel
 * the source may not be on the channel */
E void (*invite_sts)(user_t *source, user_t *target, channel_t *channel);
/* quit a client on the services server with the given message */
E void (*quit_sts)(user_t *u, char *reason);
/* send wallops
 * check config_options.silent and do nothing if it's set
 * use something that only opers can see if easily possible */
E void (*wallops)(char *fmt, ...);
/* join a channel with a client on the services server
 * the client should be introduced opped
 * isnew indicates the channel modes (and bans XXX) should be bursted
 * note that the channelts can still be old in this case (e.g. kills)
 * modes is a convenience argument giving the simple modes with parameters
 * do not rely upon chanuser_find(c,u) */
E void (*join_sts)(channel_t *c, user_t *u, boolean_t isnew, char *modes);
/* lower the TS of a channel, joining it with the given client on the
 * services server (opped), replacing the current simple modes with the
 * ones stored in the channel_t and clearing all other statuses
 * if bans are timestamped on this ircd, call chanban_clear()
 * if the topic is timestamped on this ircd, clear it */
E void (*chan_lowerts)(channel_t *c, user_t *u);
/* kick a user from a channel
 * from is a client on the services server which may or may not be
 * on the channel */
E void (*kick)(char *from, char *channel, char *to, char *reason);
/* send a privmsg
 * here it's ok to assume the source is able to send */
E void (*msg)(char *from, char *target, char *fmt, ...);
/* send a notice
 * from can be a client on the services server or the services server
 * itself (me.name or ME)
 * if the source cannot send because it is not on the channel, send the
 * notice from the server or join for a moment */
E void (*notice_sts)(char *from, char *target, char *fmt, ...);
/* send a notice to ops in a channel
 * source may or may not be on channel
 * generic_wallchops() sends an individual notice to each channel operator */
E void (*wallchops)(user_t *source, channel_t *target, char *message);
/* send a numeric from must be me.name or ME */
E void (*numeric_sts)(char *from, int numeric, char *target, char *fmt, ...);
/* kill a user
 * from can be a client on the services server or the services server
 * itself (me.name or ME)
 * it is recommended to change an invalid source to ME */
E void (*skill)(char *from, char *nick, char *fmt, ...);
/* part a channel with a client on the services server */
E void (*part)(char *chan, char *nick);
/* add a kline on the servers matching the given mask
 * duration is in seconds, 0 for a permanent kline
 * if the ircd requires klines to be sent from users, use opersvs */
E void (*kline_sts)(char *server, char *user, char *host, long duration, char *reason);
/* remove a kline on the servers matching the given mask
 * if the ircd requires unklines to be sent from users, use opersvs */
E void (*unkline_sts)(char *server, char *user, char *host);
/* make chanserv set a topic on a channel
 * setter and ts should be used if the ircd supports topics to be set
 * with a given topicsetter and topicts; ts is not a channelts */
E void (*topic_sts)(char *channel, char *setter, time_t ts, char *topic);
/* set modes on a channel by the given sender; sender must be a client
 * on the services server; sender may or may not be on channel */
E void (*mode_sts)(char *sender, char *target, char *modes);
/* ping the uplink
 * first check if me.connected is true and bail if not */
E void (*ping_sts)(void);
/* mark user 'origin' as logged in as 'user'
 * wantedhost is currently not used
 * first check if me.connected is true and bail if not */
E void (*ircd_on_login)(char *origin, char *user, char *wantedhost);
/* mark user 'origin' as logged out
 * first check if me.connected is true and bail if not
 * return FALSE if successful or logins are not supported
 * return TRUE if the user was killed to force logout (P10) */
E boolean_t (*ircd_on_logout)(char *origin, char *user, char *wantedhost);
/* introduce a fake server
 * it is ok to use opersvs to squit the old server */
E void (*jupe)(char *server, char *reason);
/* set a dynamic spoof on a user */
E void (*sethost_sts)(char *source, char *target, char *host);
/* force a nickchange for a user
 * possible values for type:
 * FNC_REGAIN: give a registered user their nick back
 * FNC_FORCE:  force a user off their nick (kill if unsupported)
 */
E void (*fnc_sts)(user_t *source, user_t *u, char *newnick, int type);
/* change nick, user, host and/or services login name for a user
 * target may also be a not yet fully introduced UID (for SASL) */
E void (*svslogin_sts)(char *target, char *nick, char *user, char *host, char *login);
/* send sasl message */
E void (*sasl_sts) (char *target, char mode, char *data);

E uint8_t generic_server_login(void);
E void generic_introduce_nick(char *nick, char *ser, char *host, char *real, char *uid);
E void generic_invite_sts(user_t *source, user_t *target, channel_t *channel);
E void generic_quit_sts(user_t *u, char *reason);
E void generic_wallops(char *fmt, ...);
E void generic_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes);
E void generic_chan_lowerts(channel_t *c, user_t *u);
E void generic_kick(char *from, char *channel, char *to, char *reason);
E void generic_msg(char *from, char *target, char *fmt, ...);
E void generic_notice(char *from, char *target, char *fmt, ...);
E void generic_wallchops(user_t *source, channel_t *target, char *message);
E void generic_numeric_sts(char *from, int numeric, char *target, char *fmt, ...);
E void generic_skill(char *from, char *nick, char *fmt, ...);
E void generic_part(char *chan, char *nick);
E void generic_kline_sts(char *server, char *user, char *host, long duration, char *reason);
E void generic_unkline_sts(char *server, char *user, char *host);
E void generic_topic_sts(char *channel, char *setter, time_t ts, char *topic);
E void generic_mode_sts(char *sender, char *target, char *modes);
E void generic_ping_sts(void);
E void generic_on_login(char *origin, char *user, char *wantedhost);
E boolean_t generic_on_logout(char *origin, char *user, char *wantedhost);
E void generic_jupe(char *server, char *reason);
E void generic_sethost_sts(char *source, char *target, char *host);
E void generic_fnc_sts(user_t *source, user_t *u, char *newnick, int type);
E void generic_svslogin_sts(char *target, char *nick, char *user, char *host, char *login);
E void generic_sasl_sts(char *target, char mode, char *data);

E struct cmode_ *mode_list;
E struct extmode *ignore_mode_list;
E struct cmode_ *status_mode_list;
E struct cmode_ *prefix_mode_list;

E ircd_t *ircd;

/* This stuff is for databases, but it's the same sort of concept. */
E void (*db_save)(void *arg);
E void (*db_load)(void);

#endif

