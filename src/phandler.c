/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * The rights to this code are as documented in doc/LICENSE.
 *
 * Generic protocol event handlers.
 *
 * $Id: phandler.c 7771 2007-03-03 12:46:36Z pippijn $
 */

#include "atheme.h"

uint8_t(*server_login) (void) = generic_server_login;
void (*introduce_nick) (user_t *u) = generic_introduce_nick;
void (*wallops_sts) (const char *text) = generic_wallops_sts;
void (*join_sts) (channel_t *c, user_t *u, boolean_t isnew, char *modes) = generic_join_sts;
void (*chan_lowerts) (channel_t *c, user_t *u) = generic_chan_lowerts;
void (*kick) (char *from, char *channel, char *to, char *reason) = generic_kick;
void (*msg) (char *from, char *target, char *fmt, ...) = generic_msg;
void (*notice_user_sts) (user_t *from, user_t *target, const char *text) = generic_notice_user_sts;
void (*notice_global_sts) (user_t *from, const char *mask, const char *text) = generic_notice_global_sts;
void (*notice_channel_sts) (user_t *from, channel_t *target, const char *text) = generic_notice_channel_sts;
void (*wallchops) (user_t *source, channel_t *target, char *message) = generic_wallchops;
void (*numeric_sts) (char *from, int numeric, char *target, char *fmt, ...) = generic_numeric_sts;
void (*skill) (char *from, char *nick, char *fmt, ...) = generic_skill;
void (*part) (char *chan, char *nick) = generic_part;
void (*kline_sts) (char *server, char *user, char *host, long duration, char *reason) = generic_kline_sts;
void (*unkline_sts) (char *server, char *user, char *host) = generic_unkline_sts;
void (*topic_sts) (channel_t *c, char *setter, time_t ts, time_t prevts, char *topic) = generic_topic_sts;
void (*mode_sts) (char *sender, char *target, char *modes) = generic_mode_sts;
void (*ping_sts) (void) = generic_ping_sts;
void (*quit_sts) (user_t *u, char *reason) = generic_quit_sts;
void (*ircd_on_login) (char *origin, char *user, char *wantedhost) = generic_on_login;
boolean_t (*ircd_on_logout) (char *origin, char *user, char *wantedhost) = generic_on_logout;
void (*jupe) (char *server, char *reason) = generic_jupe;
void (*sethost_sts) (char *source, char *target, char *host) = generic_sethost_sts;
void (*fnc_sts) (user_t *source, user_t *u, char *newnick, int type) = generic_fnc_sts;
void (*holdnick_sts)(user_t *source, int duration, const char *nick, myuser_t *account) = generic_holdnick_sts;
void (*invite_sts) (user_t *source, user_t *target, channel_t *channel) = generic_invite_sts;
void (*svslogin_sts) (char *target, char *nick, char *user, char *host, char *login) = generic_svslogin_sts;
void (*sasl_sts) (char *target, char mode, char *data) = generic_sasl_sts;

uint8_t generic_server_login(void)
{
	/* Nothing to do here. */
	return 0;
}

void generic_introduce_nick(user_t *u)
{
	/* Nothing to do here. */
}

void generic_wallops_sts(const char *text)
{
	slog(LG_INFO, "Don't know how to send wallops: %s", text);
}

void generic_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	/* We can't do anything here. Bail. */
}

void generic_chan_lowerts(channel_t *c, user_t *u)
{
	slog(LG_ERROR, "chan_lowerts() called but not supported!");
	join_sts(c, u, TRUE, channel_modes(c, TRUE));
}

void generic_kick(char *from, char *channel, char *to, char *reason)
{
	/* We can't do anything here. Bail. */
}

void generic_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	slog(LG_INFO, "Cannot send message to %s (%s): don't know how. Load a protocol module perhaps?", target, buf);
}

void generic_notice_user_sts(user_t *from, user_t *target, const char *text)
{
	slog(LG_INFO, "Cannot send notice to %s (%s): don't know how. Load a protocol module perhaps?", target->nick, text);
}

void generic_notice_global_sts(user_t *from, const char *mask, const char *text)
{
	slog(LG_INFO, "Cannot send global notice to %s (%s): don't know how. Load a protocol module perhaps?", mask, text);
}

void generic_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	slog(LG_INFO, "Cannot send notice to %s (%s): don't know how. Load a protocol module perhaps?", target->name, text);
}

void generic_wallchops(user_t *sender, channel_t *channel, char *message)	
{
	/* ugly, but always works -- jilles */
	node_t *n;
	chanuser_t *cu;

	LIST_FOREACH(n, channel->members.head)
	{
		cu = (chanuser_t *)n->data;
		if (cu->user->server != me.me && cu->modes & CMODE_OP)
			notice(sender->nick, cu->user->nick, "[@%s] %s", channel->name, message);
	}
}

void generic_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	/* cant do anything here. bail. */
}

void generic_skill(char *from, char *nick, char *fmt, ...)
{
	/* cant do anything here. bail. */
}

void generic_part(char *chan, char *nick)
{
	/* cant do anything here. bail. */
}

void generic_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	/* cant do anything here. bail. */
}

void generic_unkline_sts(char *server, char *user, char *host)
{
	/* cant do anything here. bail. */
}

void generic_topic_sts(channel_t *c, char *setter, time_t ts, time_t prevts, char *topic)
{
	/* cant do anything here. bail. */
}

void generic_mode_sts(char *sender, char *target, char *modes)
{
	/* cant do anything here. bail. */
}

void generic_ping_sts(void)
{
	/* cant do anything here. bail. */
}

void generic_quit_sts(user_t *u, char *reason)
{
	/* cant do anything here. bail. */
}

void generic_on_login(char *origin, char *user, char *wantedhost)
{
	/* nothing to do here. */
}

boolean_t generic_on_logout(char *origin, char *user, char *wantedhost)
{
	/* nothing to do here. */
	return FALSE;
}

void generic_jupe(char *server, char *reason)
{
	/* nothing to do here. */
}

void generic_sethost_sts(char *source, char *target, char *host)
{
	/* nothing to do here. */
}

void generic_fnc_sts(user_t *source, user_t *u, char *newnick, int type)
{
	if (type == FNC_FORCE)
		skill(source->nick, u->nick, "Nickname enforcement (%s)", u->nick);
}

E void generic_holdnick_sts(user_t *source, int duration, const char *nick, myuser_t *account)
{
	/* nothing to do here. */	
}

void generic_invite_sts(user_t *source, user_t *target, channel_t *channel)
{
	/* nothing to do here. */	
}

void generic_svslogin_sts(char *target, char *nick, char *user, char *host, char *login)
{
	/* nothing to do here. */
}

void generic_sasl_sts(char *target, char mode, char *data)
{
	/* nothing to do here. */
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
