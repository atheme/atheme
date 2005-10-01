/*
 * Copyright (c) 2005 Atheme Development Group
 * The rights to this code are as documented in doc/LICENSE.
 *
 * Generic protocol event handlers.
 *
 * $Id: phandler.c 2497 2005-10-01 04:35:25Z nenolod $
 */

#include "atheme.h"

uint8_t(*server_login) (void) = generic_server_login;
user_t *(*introduce_nick) (char *nick, char *user, char *host, char *real, char *modes) = generic_introduce_nick;
void (*wallops) (char *fmt, ...) = generic_wallops;
void (*join) (char *chan, char *nick) = generic_join;
void (*kick) (char *from, char *channel, char *to, char *reason) = generic_kick;
void (*msg) (char *from, char *target, char *fmt, ...) = generic_msg;
void (*notice) (char *from, char *target, char *fmt, ...) = generic_notice;
void (*numeric_sts) (char *from, int numeric, char *target, char *fmt, ...) = generic_numeric_sts;
void (*skill) (char *from, char *nick, char *fmt, ...) = generic_skill;
void (*part) (char *chan, char *nick) = generic_part;
void (*kline_sts) (char *server, char *user, char *host, long duration, char *reason) = generic_kline_sts;
void (*unkline_sts) (char *server, char *user, char *host) = generic_unkline_sts;
void (*topic_sts) (char *channel, char *setter, char *topic) = generic_topic_sts;
void (*mode_sts) (char *sender, char *target, char *modes) = generic_mode_sts;
void (*ping_sts) (void) = generic_ping_sts;
void (*quit_sts) (user_t *u, char *reason) = generic_quit_sts;
void (*ircd_on_login) (char *origin, char *user, char *wantedhost) = generic_on_login;
void (*ircd_on_logout) (char *origin, char *user, char *wantedhost) = generic_on_logout;
void (*jupe) (char *server, char *reason) = generic_jupe;

uint8_t generic_server_login(void)
{
	/* Nothing to do here. */
	return 0;
}

user_t *generic_introduce_nick(char *nick, char *ser, char *host, char *real, char *modes)
{
	/* Nothing to do here. */
	return NULL;
}

void generic_wallops(char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	if (config_options.silent)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	slog(LG_INFO, "Don't know how to send wallops: %s", buf);
}

void generic_join(char *chan, char *nick)
{
	/* We can't do anything here. Bail. */
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

void generic_notice(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	slog(LG_INFO, "Cannot send notice to %s (%s): don't know how. Load a protocol module perhaps?", target, buf);
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

void generic_topic_sts(char *channel, char *setter, char *topic)
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

void generic_on_logout(char *origin, char *user, char *wantedhost)
{
	/* nothing to do here. */
}

void generic_jupe(char *server, char *reason)
{
	/* nothing to do here. */
}
