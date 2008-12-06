/*
 * atheme-services: A collection of minimalist IRC services
 * phandler.c: Generic protocol handling routines.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"
#include "uplink.h"

unsigned int(*server_login) (void) = generic_server_login;
void (*introduce_nick) (user_t *u) = generic_introduce_nick;
void (*wallops_sts) (const char *text) = generic_wallops_sts;
void (*join_sts) (channel_t *c, user_t *u, boolean_t isnew, char *modes) = generic_join_sts;
void (*chan_lowerts) (channel_t *c, user_t *u) = generic_chan_lowerts;
void (*kick) (user_t *source, channel_t *c, user_t *u, const char *reason) = generic_kick;
void (*msg) (const char *from, const char *target, const char *fmt, ...) = generic_msg;
void (*notice_user_sts) (user_t *from, user_t *target, const char *text) = generic_notice_user_sts;
void (*notice_global_sts) (user_t *from, const char *mask, const char *text) = generic_notice_global_sts;
void (*notice_channel_sts) (user_t *from, channel_t *target, const char *text) = generic_notice_channel_sts;
void (*wallchops) (user_t *source, channel_t *target, const char *message) = generic_wallchops;
void (*numeric_sts) (server_t *from, int numeric, user_t *target, const char *fmt, ...) = generic_numeric_sts;
void (*kill_id_sts) (user_t *killer, const char *id, const char *reason) = generic_kill_id_sts;
void (*part_sts) (channel_t *c, user_t *u) = generic_part_sts;
void (*kline_sts) (char *server, char *user, char *host, long duration, char *reason) = generic_kline_sts;
void (*unkline_sts) (char *server, char *user, char *host) = generic_unkline_sts;
void (*topic_sts) (channel_t *c, const char *setter, time_t ts, time_t prevts, const char *topic) = generic_topic_sts;
void (*mode_sts) (char *sender, channel_t *target, char *modes) = generic_mode_sts;
void (*ping_sts) (void) = generic_ping_sts;
void (*quit_sts) (user_t *u, const char *reason) = generic_quit_sts;
void (*ircd_on_login) (char *origin, char *user, char *wantedhost) = generic_on_login;
boolean_t (*ircd_on_logout) (char *origin, char *user, char *wantedhost) = generic_on_logout;
void (*jupe) (const char *server, const char *reason) = generic_jupe;
void (*sethost_sts) (char *source, char *target, char *host) = generic_sethost_sts;
void (*fnc_sts) (user_t *source, user_t *u, char *newnick, int type) = generic_fnc_sts;
void (*holdnick_sts)(user_t *source, int duration, const char *nick, myuser_t *account) = generic_holdnick_sts;
void (*invite_sts) (user_t *source, user_t *target, channel_t *channel) = generic_invite_sts;
void (*svslogin_sts) (char *target, char *nick, char *user, char *host, char *login) = generic_svslogin_sts;
void (*sasl_sts) (char *target, char mode, char *data) = generic_sasl_sts;
node_t *(*next_matching_ban)(channel_t *c, user_t *u, int type, node_t *first) = generic_next_matching_ban;
node_t *(*next_matching_host_chanacs)(mychan_t *mc, user_t *u, node_t *first) = generic_next_matching_host_chanacs;
boolean_t (*is_valid_host)(const char *host) = generic_is_valid_host;

unsigned int generic_server_login(void)
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

void generic_kick(user_t *source, channel_t *c, user_t *u, const char *reason)
{
	/* We can't do anything here. Bail. */
}

void generic_msg(const char *from, const char *target, const char *fmt, ...)
{
	va_list ap;
	char *buf;

	va_start(ap, fmt);
	vasprintf(&buf, fmt, ap);
	va_end(ap);

	slog(LG_INFO, "Cannot send message to %s (%s): don't know how. Load a protocol module perhaps?", target, buf);
	free(buf);
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

void generic_wallchops(user_t *sender, channel_t *channel, const char *message)	
{
	/* ugly, but always works -- jilles */
	node_t *n;
	chanuser_t *cu;

	LIST_FOREACH(n, channel->members.head)
	{
		cu = (chanuser_t *)n->data;
		if (cu->user->server != me.me && cu->modes & CSTATUS_OP)
			notice(sender->nick, cu->user->nick, "[@%s] %s", channel->name, message);
	}
}

void generic_numeric_sts(server_t *from, int numeric, user_t *target, const char *fmt, ...)
{
	va_list va;
	char *buf;

	va_start(va, fmt);
	vasprintf(&buf, fmt, va);
	va_end(va);

	sts(":%s %d %s %s", SERVER_NAME(from), numeric, CLIENT_NAME(target), buf);
	free(buf);
}

void generic_kill_id_sts(user_t *killer, const char *id, const char *reason)
{
	/* cant do anything here. bail. */
}

void generic_part_sts(channel_t *c, user_t *u)
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

void generic_topic_sts(channel_t *c, const char *setter, time_t ts, time_t prevts, const char *topic)
{
	/* cant do anything here. bail. */
}

void generic_mode_sts(char *sender, channel_t *target, char *modes)
{
	/* cant do anything here. bail. */
}

void generic_ping_sts(void)
{
	/* cant do anything here. bail. */
}

void generic_quit_sts(user_t *u, const char *reason)
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

void generic_jupe(const char *server, const char *reason)
{
	/* nothing to do here. */
}

void generic_sethost_sts(char *source, char *target, char *host)
{
	/* nothing to do here. */
}

void generic_fnc_sts(user_t *source, user_t *u, char *newnick, int type)
{
	if (type == FNC_FORCE) /* XXX this does not work properly */
		kill_id_sts(source, CLIENT_NAME(u), "Nickname enforcement");
}

void generic_holdnick_sts(user_t *source, int duration, const char *nick, myuser_t *account)
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

node_t *generic_next_matching_ban(channel_t *c, user_t *u, int type, node_t *first)
{
	chanban_t *cb;
	node_t *n;
	char hostbuf[NICKLEN+USERLEN+HOSTLEN];
	char realbuf[NICKLEN+USERLEN+HOSTLEN];
	char ipbuf[NICKLEN+USERLEN+HOSTLEN];

	snprintf(hostbuf, sizeof hostbuf, "%s!%s@%s", u->nick, u->user, u->vhost);
	snprintf(realbuf, sizeof realbuf, "%s!%s@%s", u->nick, u->user, u->host);
	/* will be nick!user@ if ip unknown, doesn't matter */
	snprintf(ipbuf, sizeof ipbuf, "%s!%s@%s", u->nick, u->user, u->ip);
	LIST_FOREACH(n, first)
	{
		cb = n->data;

		if (cb->type == type &&
				(!match(cb->mask, hostbuf) || !match(cb->mask, realbuf) || !match(cb->mask, ipbuf) || (ircd->flags & IRCD_CIDR_BANS && !match_cidr(cb->mask, ipbuf))))
			return n;
	}
	return NULL;
}

node_t *generic_next_matching_host_chanacs(mychan_t *mc, user_t *u, node_t *first)
{
	chanacs_t *ca;
	node_t *n;
	char hostbuf[NICKLEN+USERLEN+HOSTLEN];
	char ipbuf[NICKLEN+USERLEN+HOSTLEN];

	snprintf(hostbuf, sizeof hostbuf, "%s!%s@%s", u->nick, u->user, u->vhost);
	/* will be nick!user@ if ip unknown, doesn't matter */
	snprintf(ipbuf, sizeof ipbuf, "%s!%s@%s", u->nick, u->user, u->ip);

	LIST_FOREACH(n, first)
	{
		ca = n->data;

		if (ca->myuser != NULL)
		       continue;
		if (!match(ca->host, hostbuf) || !match(ca->host, ipbuf) || (ircd->flags & IRCD_CIDR_BANS && !match_cidr(ca->host, ipbuf)))
			return n;
	}
	return NULL;
}

boolean_t generic_is_valid_host(const char *host)
{
	/* don't know what to do here */
	return TRUE;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
