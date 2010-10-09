/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol handlers, both generic and the actual declarations themselves.
 *
 */

#ifndef PHANDLER_H
#define PHANDLER_H

struct ircd_ {
	const char *ircdname;
	const char *tldprefix;
	bool uses_uid;
	bool uses_rcommand;
	bool uses_owner;
	bool uses_protect;
	bool uses_halfops;
	bool uses_p10;		/* Parser hackhack. */
	bool uses_vhost;		/* Do we use vHosts? */
	unsigned int oper_only_modes;
	unsigned int owner_mode;
	unsigned int protect_mode;
	unsigned int halfops_mode;
	const char *owner_mchar;
	const char *protect_mchar;
	const char *halfops_mchar;
	unsigned int type;
	unsigned int perm_mode;		/* Modes to not disappear when empty */
	unsigned int oimmune_mode;	/* Mode to disallow kicking ircops */
	const char *ban_like_modes;		/* e.g. "beI" */
	char except_mchar;
	char invex_mchar;
	int flags;
};

typedef struct ircd_ ircd_t;

/* values for type */
/*  -- what the HELL are these used for? A grep reveals nothing.. --w00t
 *  -- they are used to provide a hint to third-party module coders about what
 *     ircd they are working with. --nenolod
 */
#define PROTOCOL_ASUKA			1
#define PROTOCOL_BAHAMUT		2
#define PROTOCOL_CHARYBDIS		3
#define PROTOCOL_DREAMFORGE		4
#define PROTOCOL_HYPERION		5 /* obsolete */
#define PROTOCOL_INSPIRCD		6
#define PROTOCOL_IRCNET			7
#define PROTOCOL_MONKEY			8 /* obsolete */
#define PROTOCOL_PLEXUS			9
#define PROTOCOL_PTLINK			10
#define PROTOCOL_RATBOX			11
#define PROTOCOL_SCYLLA			12
#define PROTOCOL_SHADOWIRCD		13
#define PROTOCOL_SORCERY		14 /* obsolete */
#define PROTOCOL_ULTIMATE3		15
#define PROTOCOL_UNDERNET		16
#define PROTOCOL_UNREAL			17
#define PROTOCOL_SOLIDIRCD		18
#define PROTOCOL_NEFARIOUS		19
#define PROTOCOL_OFFICEIRC		20

#define PROTOCOL_OTHER			255

/* values for flags */
#define IRCD_CIDR_BANS			1
#define IRCD_HOLDNICK			2 /* supports holdnick_sts() */
#define IRCD_TOPIC_NOCOLOUR		4

/* forced nick change types */
#define FNC_REGAIN 0 /* give a registered user their nick back */
#define FNC_FORCE  1 /* force a user off their nick (kill if unsupported) */

/* server login, usually sends PASS, CAPAB, SERVER and SVINFO
 * you can still change ircd->uses_uid at this point
 * set me.bursting = true
 * return 1 if sts() failed (by returning 1), otherwise 0 */
E unsigned int (*server_login)(void);
/* introduce a client on the services server */
E void (*introduce_nick)(user_t *u);
/* send an invite for a given user to a channel
 * the source may not be on the channel */
E void (*invite_sts)(user_t *source, user_t *target, channel_t *channel);
/* quit a client on the services server with the given message */
E void (*quit_sts)(user_t *u, const char *reason);
/* send wallops
 * use something that only opers can see if easily possible */
E void (*wallops_sts)(const char *text);
/* join a channel with a client on the services server
 * the client should be introduced opped
 * isnew indicates the channel modes (and bans XXX) should be bursted
 * note that the channelts can still be old in this case (e.g. kills)
 * modes is a convenience argument giving the simple modes with parameters
 * do not rely upon chanuser_find(c,u) */
E void (*join_sts)(channel_t *c, user_t *u, bool isnew, char *modes);
/* lower the TS of a channel, joining it with the given client on the
 * services server (opped), replacing the current simple modes with the
 * ones stored in the channel_t and clearing all other statuses
 * if bans are timestamped on this ircd, call chanban_clear()
 * if the topic is timestamped on this ircd, clear it */
E void (*chan_lowerts)(channel_t *c, user_t *u);
/* kick a user from a channel
 * source is a client on the services server which may or may not be
 * on the channel */
E void (*kick)(user_t *source, channel_t *c, user_t *u, const char *reason);
/* send a privmsg
 * here it's ok to assume the source is able to send */
E void (*msg)(const char *from, const char *target, const char *fmt, ...) PRINTFLIKE(3, 4);
/* send a global privmsg to all users on servers matching the mask
 * from is a client on the services server
 * mask is either "*" or it has a non-wildcard TLD */
E void (*msg_global_sts)(user_t *from, const char *mask, const char *text);
/* send a notice to a user
 * from can be a client on the services server or the services server
 * itself (NULL) */
E void (*notice_user_sts)(user_t *from, user_t *target, const char *text);
/* send a global notice to all users on servers matching the mask
 * from is a client on the services server
 * mask is either "*" or it has a non-wildcard TLD */
E void (*notice_global_sts)(user_t *from, const char *mask, const char *text);
/* send a notice to a channel
 * from can be a client on the services server or the services server
 * itself (NULL)
 * if the source cannot send because it is not on the channel, send the
 * notice from the server or join for a moment */
E void (*notice_channel_sts)(user_t *from, channel_t *target, const char *text);
/* send a notice to ops in a channel
 * source may or may not be on channel
 * generic_wallchops() sends an individual notice to each channel operator */
E void (*wallchops)(user_t *source, channel_t *target, const char *message);
/* send a numeric from must currently be me.me */
E void (*numeric_sts)(server_t *from, int numeric, user_t *target, const char *fmt, ...) PRINTFLIKE(4, 5);
/* kill a user
 * killer can be a client on the services server or NULL for the
 * services server itself
 * unlike other functions, the target is specified by a UID or nick;
 * do not call user_find(), user_find_named() or similar on it */
E void (*kill_id_sts)(user_t *killer, const char *id, const char *reason);
/* part a channel with a client on the services server */
E void (*part_sts)(channel_t *c, user_t *u);
/* add a kline on the servers matching the given mask
 * duration is in seconds, 0 for a permanent kline
 * if the ircd requires klines to be sent from users, use opersvs */
E void (*kline_sts)(const char *server, const char *user, const char *host, long duration, const char *reason);
/* remove a kline on the servers matching the given mask
 * if the ircd requires unklines to be sent from users, use opersvs */
E void (*unkline_sts)(const char *server, const char *user, const char *host);
/* add a xline on the servers matching the given mask
 * duration is in seconds, 0 for a permanent xline
 * if the ircd requires xlines to be sent from users, use opersvs */
E void (*xline_sts)(const char *server, const char *realname, long duration, const char *reason);
/* remove a xline on the servers matching the given mask
 * if the ircd requires unxlines to be sent from users, use opersvs */
E void (*unxline_sts)(const char *server, const char *realname);
/* add a qline on the servers matching the given mask
 * duration is in seconds, 0 for a permanent qline
 * if the ircd requires qlines to be sent from users, use opersvs */
E void (*qline_sts)(const char *server, const char *mask, long duration, const char *reason);
/* remove a qline on the servers matching the given mask
 * if the ircd requires unqlines to be sent from users, use opersvs */
E void (*unqline_sts)(const char *server, const char *mask);
/* make the given service set a topic on a channel
 * setter and ts should be used if the ircd supports topics to be set
 * with a given topicsetter and topicts; ts is not a channelts
 * prevts is the topicts of the old topic or 0 if there was no topic,
 * useful in optimizing which form of topic change to use
 * if the given topicts was not set and topicts is used on the ircd,
 * set c->topicts to the value used */
E void (*topic_sts)(channel_t *c, user_t *source, const char *setter, time_t ts, time_t prevts, const char *topic);
/* set modes on a channel by the given sender; sender must be a client
 * on the services server; sender may or may not be on channel */
E void (*mode_sts)(char *sender, channel_t *target, char *modes);
/* ping the uplink
 * first check if me.connected is true and bail if not */
E void (*ping_sts)(void);
/* mark user 'u' as logged in as 'account'
 * wantedhost is currently not used
 * first check if me.connected is true and bail if not */
E void (*ircd_on_login)(user_t *u, myuser_t *account, const char *wantedhost);
/* mark user 'u' as logged out
 * first check if me.connected is true and bail if not
 * return false if successful or logins are not supported
 * return true if the user was killed to force logout (P10) */
E bool (*ircd_on_logout)(user_t *u, const char *account);
/* introduce a fake server
 * it is ok to use opersvs to squit the old server
 * if SQUIT uses kill semantics (e.g. charybdis), server_delete() the server
 * and continue
 * if SQUIT uses unconnect semantics (e.g. bahamut), set SF_JUPE_PENDING on
 * the server and return; you will be called again when the server is really
 * deleted */
E void (*jupe)(const char *server, const char *reason);
/* set a dynamic spoof on a user
 * if the ircd does not notify the user of this, do
 * notice(source->nick, target->nick, "Setting your host to \2%s\2.", host); */
E void (*sethost_sts)(user_t *source, user_t *target, const char *host);
/* force a nickchange for a user
 * possible values for type:
 * FNC_REGAIN: give a registered user their nick back
 * FNC_FORCE:  force a user off their nick (kill if unsupported)
 */
E void (*fnc_sts)(user_t *source, user_t *u, char *newnick, int type);
/* temporarily make a nick unavailable to users
 * source is the responsible service
 * duration is in seconds, 0 to remove the effect of a previous call
 * account is an account that may still use the nick, or NULL */
E void (*holdnick_sts)(user_t *source, int duration, const char *nick, myuser_t *account);
/* change nick, user, host and/or services login name for a user
 * target may also be a not yet fully introduced UID (for SASL) */
E void (*svslogin_sts)(char *target, char *nick, char *user, char *host, char *login);
/* send sasl message */
E void (*sasl_sts) (char *target, char mode, char *data);
/* find next channel ban (or other ban-like mode) matching user */
E mowgli_node_t *(*next_matching_ban)(channel_t *c, user_t *u, int type, mowgli_node_t *first);
/* find next host channel access matching user */
E mowgli_node_t *(*next_matching_host_chanacs)(mychan_t *mc, user_t *u, mowgli_node_t *first);
/* check a vhost for validity; the core will already have checked for
 * @!?*, space, empty, : at start, length and cidr masks */
E bool (*is_valid_host)(const char *host);
/* burst a channel mlock. */
E void (*mlock_sts)(channel_t *c);

E unsigned int generic_server_login(void);
E void generic_introduce_nick(user_t *u);
E void generic_invite_sts(user_t *source, user_t *target, channel_t *channel);
E void generic_quit_sts(user_t *u, const char *reason);
E void generic_wallops_sts(const char *text);
E void generic_join_sts(channel_t *c, user_t *u, bool isnew, char *modes);
E void generic_chan_lowerts(channel_t *c, user_t *u);
E void generic_kick(user_t *source, channel_t *c, user_t *u, const char *reason);
E void generic_msg(const char *from, const char *target, const char *fmt, ...);
E void generic_msg_global_sts(user_t *from, const char *mask, const char *text);
E void generic_notice_user_sts(user_t *from, user_t *target, const char *text);
E void generic_notice_global_sts(user_t *from, const char *mask, const char *text);
E void generic_notice_channel_sts(user_t *from, channel_t *target, const char *text);
E void generic_wallchops(user_t *source, channel_t *target, const char *message);
E void generic_numeric_sts(server_t *from, int numeric, user_t *target, const char *fmt, ...);
E void generic_kill_id_sts(user_t *killer, const char *id, const char *reason);
E void generic_part_sts(channel_t *c, user_t *u);
E void generic_kline_sts(const char *server, const char *user, const char *host, long duration, const char *reason);
E void generic_unkline_sts(const char *server, const char *user, const char *host);
E void generic_xline_sts(const char *server, const char *realname, long duration, const char *reason);
E void generic_unxline_sts(const char *server, const char *realname);
E void generic_qline_sts(const char *server, const char *mask, long duration, const char *reason);
E void generic_unqline_sts(const char *server, const char *mask);
E void generic_topic_sts(channel_t *c, user_t *source, const char *setter, time_t ts, time_t prevts, const char *topic);
E void generic_mode_sts(char *sender, channel_t *target, char *modes);
E void generic_ping_sts(void);
E void generic_on_login(user_t *u, myuser_t *account, const char *wantedhost);
E bool generic_on_logout(user_t *u, const char *account);
E void generic_jupe(const char *server, const char *reason);
E void generic_sethost_sts(user_t *source, user_t *target, const char *host);
E void generic_fnc_sts(user_t *source, user_t *u, char *newnick, int type);
E void generic_holdnick_sts(user_t *source, int duration, const char *nick, myuser_t *account);
E void generic_svslogin_sts(char *target, char *nick, char *user, char *host, char *login);
E void generic_sasl_sts(char *target, char mode, char *data);
E mowgli_node_t *generic_next_matching_ban(channel_t *c, user_t *u, int type, mowgli_node_t *first);
E mowgli_node_t *generic_next_matching_host_chanacs(mychan_t *mc, user_t *u, mowgli_node_t *first);
E bool generic_is_valid_host(const char *host);
E void generic_mlock_sts(channel_t *c);

E struct cmode_ *mode_list;
E struct extmode *ignore_mode_list;
E size_t ignore_mode_list_size;
E struct cmode_ *status_mode_list;
E struct cmode_ *prefix_mode_list;
E struct cmode_ *user_mode_list;

E ircd_t *ircd;

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
