/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * This file contains functionality which implements
 * the OperServ AKILL command.
 */

#include <atheme.h>

#include <errno.h>

static mowgli_patricia_t *os_akill_cmds = NULL;

// The longest timed AKILL that can be added (anything longer overflows or is meant to be !P)
#define AKILL_MAX_SECONDS       (20L * 365L * 24L * 60L * 60L)

/*
 * Parse a duration given as "<number>[h|d|w]" (default unit: minutes).
 *
 * Returns the duration in seconds, 0 if it is not a valid duration and -1 if
 * it is valid but too long. Nothing here can overflow.
 */
static long
akill_parse_duration(const char *s)
{
	char *end = NULL;
	long long value, unit = SECONDS_PER_MINUTE;

	errno = 0;
	value = strtoll(s, &end, 10);

	if (errno != 0 || end == s || value <= 0)
		return 0;

	if (*end == 'h' || *end == 'H')
		unit *= MINUTES_PER_HOUR;
	else if (*end == 'd' || *end == 'D')
		unit *= MINUTES_PER_DAY;
	else if (*end == 'w' || *end == 'W')
		unit *= MINUTES_PER_WEEK;
	else if (*end != '\0')
		return 0;

	if (value > AKILL_MAX_SECONDS / unit)
		return -1;

	return (long) (value * unit);
}

/*
 * True if HOST is a literal IPv6 loopback address in any spelling: ::1,
 * 0:0:0:0:0:0:0:1, 0::0001, ::ffff:127.0.0.1, ::ffff:7f00:1 ...
 * (A plain string comparison with "::1" only finds one of them.)
 */
static bool
akill_is_ipv6_loopback(const char *host)
{
	static const unsigned char mapped[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };
	struct in6_addr addr;

	if (inet_pton(AF_INET6, host, &addr) != 1)
		return false;

	if (IN6_IS_ADDR_LOOPBACK(&addr))
		return true;

	return memcmp(&addr, mapped, sizeof mapped) == 0 && addr.s6_addr[12] == 127;
}

/*
 * Parse "<number>" or "<first>:<last>" (decimal digits only).
 *
 * Anything else is NOT a number: an IPv6 address ("2001:db8::1"), an IPv4
 * address ("1.2.3.4"), a host name or user@host. The old code looked for a
 * ':' or fed the string to atoi(), so those were taken for AKILL numbers.
 */
static bool
akill_parse_numbers(const char *s, unsigned long *first, unsigned long *last)
{
	char *end = NULL;

	if (!isdigit((unsigned char) *s))
		return false;

	errno = 0;
	*first = strtoul(s, &end, 10);

	if (errno != 0)
		return false;

	if (*end == '\0')
	{
		*last = *first;
		return true;
	}

	if (*end != ':' || !isdigit((unsigned char) end[1]))
		return false;

	errno = 0;
	*last = strtoul(end + 1, &end, 10);

	return errno == 0 && *end == '\0';
}

static void
os_akill_newuser(struct hook_user_nick *data)
{
	struct user *u = data->u;
	struct kline *k;

	// If the user has been killed, don't do anything.
	if (!u)
		return;

	if (is_internal_client(u))
		return;
	k = kline_find_user(u);
	if (k != NULL)
	{
		/* Server didn't have that kline, send it again.
		 * To ensure kline exempt works on akills too, do
		 * not send a KILL. -- jilles */
		char reason[BUFSIZE];
		snprintf(reason, sizeof(reason), "[#%lu] %s", k->number, k->reason);
		if (! (u->flags & UF_KLINESENT)) {
			kline_sts("*", k->user, k->host, k->duration ? k->expires - CURRTIME : 0, reason);
			u->flags |= UF_KLINESENT;
		}
	}
}

static void
os_cmd_akill(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKILL");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: AKILL ADD|DEL|LIST"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, os_akill_cmds, "AKILL");
}

static void
os_cmd_akill_add(struct sourceinfo *si, int parc, char *parv[])
{
	struct user *u;
	char *target = parv[0];
	char *token = parv[1] ? strtok(parv[1], " ") : NULL;
	char star[] = "*";
	const char *kuser, *khost;
	char *treason, reason[BUFSIZE];
	long duration;
	char *s;
	struct kline *k;

	if (!target || !token)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKILL ADD");
		command_fail(si, fault_needmoreparams, _("Syntax: AKILL ADD <nick|hostmask> [!P|!T <minutes>] <reason>"));
		return;
	}

	if (!strcasecmp(token, "!P"))
	{
		duration = 0;
		treason = strtok(NULL, "");

		if (treason)
			mowgli_strlcpy(reason, treason, BUFSIZE);
		else
			mowgli_strlcpy(reason, "No reason given", BUFSIZE);
	}
	else if (!strcasecmp(token, "!T"))
	{
		s = strtok(NULL, " ");
		treason = strtok(NULL, "");
		if (treason)
			mowgli_strlcpy(reason, treason, BUFSIZE);
		else
			mowgli_strlcpy(reason, "No reason given", BUFSIZE);
		if (s)
		{
			duration = akill_parse_duration(s);

			if (duration < 0)
			{
				command_fail(si, fault_badparams, _("The duration is too long (maximum is 20 years); use !P for a permanent AKILL."));
				return;
			}
			if (duration == 0)
			{
				command_fail(si, fault_badparams, _("Invalid duration given."));
				command_fail(si, fault_badparams, _("Syntax: AKILL ADD <nick|hostmask> [!P|!T <minutes>] <reason>"));
				return;
			}
		}
		else {
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKILL ADD");
			command_fail(si, fault_needmoreparams, _("Syntax: AKILL ADD <nick|hostmask> [!P|!T <minutes>] <reason>"));
			return;
		}

	}
	else
	{
		duration = config_options.kline_time;
		mowgli_strlcpy(reason, token, BUFSIZE);
		treason = strtok(NULL, "");

		if (treason)
		{
			mowgli_strlcat(reason, " ", BUFSIZE);
			mowgli_strlcat(reason, treason, BUFSIZE);
		}
	}

	if (strchr(target,'!'))
	{
		command_fail(si, fault_badparams, _("Invalid character '%c' in user@host."), '!');
		return;
	}

	if (!(strchr(target, '@')))
	{
		if (!(u = user_find_named(target)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), target);
			return;
		}

		if (is_internal_client(u) || u == si->su)
		{
			command_fail(si, fault_noprivs, _("\2%s\2 cannot be AKILLed."), u->nick);
			return;
		}

		kuser = star;
		khost = u->host;
	}
	else
	{
		const char *p;
		int i = 0;

		kuser = collapse(strtok(target, "@"));
		khost = collapse(strtok(NULL, ""));

		if (!kuser || !khost)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKILL ADD");
			command_fail(si, fault_needmoreparams, _("Syntax: AKILL ADD <user>@<host> [options] <reason>"));
			return;
		}

		if (strchr(khost,'@'))
		{
			command_fail(si, fault_badparams, _("Too many '%c' in user@host."), '@');
			command_fail(si, fault_badparams, _("Syntax: AKILL ADD <user>@<host> [options] <reason>"));
			return;
		}

		/* make sure there's at least 4 non-wildcards */
		/* except if the user has no wildcards */
		for (p = kuser; *p; p++)
		{
			if (*p != '*' && *p != '?' && *p != '.')
				i++;
		}
		for (p = khost; *p; p++)
		{
			if (*p != '*' && *p != '?' && *p != '.')
				i++;
		}

		if (i < 4 && (strchr(kuser, '*') || strchr(kuser, '?')) && !has_priv(si, PRIV_AKILL_ANYMASK))
		{
			command_fail(si, fault_badparams, _("Invalid user@host: \2%s@%s\2. At least four non-wildcard characters are required."), kuser, khost);
			return;
		}
	}

	if (!strcmp(kuser, "*"))
	{
		bool unsafe = false;
		char *p;

		if (!match(khost, "127.0.0.1") || !match_ips(khost, "127.0.0.1"))
			unsafe = true;
		else if (!match(khost, "::1") || !match_ips(khost, "::1") || akill_is_ipv6_loopback(khost))
			unsafe = true;
		else if (me.vhost != NULL && (!match(khost, me.vhost) || !match_ips(khost, me.vhost)))
			unsafe = true;
		else if ((p = strrchr(khost, '/')) != NULL && IsDigit(p[1]) && atoi(p + 1) < 4)
			unsafe = true;
		if (unsafe)
		{
			command_fail(si, fault_badparams, _("Invalid user@host: \2%s@%s\2. This mask is unsafe."), kuser, khost);
			logcommand(si, CMDLOG_ADMIN, "failed AKILL ADD \2%s@%s\2 (unsafe mask)", kuser, khost);
			return;
		}
	}

	if (kline_find(kuser, khost))
	{
		command_fail(si, fault_nochange, _("AKILL \2%s@%s\2 is already matched in the database."), kuser, khost);
		return;
	}

	k = kline_add(kuser, khost, reason, duration, get_storage_oper_name(si));

	if (duration)
		command_success_nodata(si, _("Timed AKILL on \2%s@%s\2 was successfully added and will expire in %s."), k->user, k->host, timediff(duration));
	else
		command_success_nodata(si, _("AKILL on \2%s@%s\2 was successfully added."), k->user, k->host);

	verbose_wallops("\2%s\2 is \2adding\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2", get_oper_name(si), k->user, k->host,
		k->reason);

	if (duration)
		logcommand(si, CMDLOG_ADMIN, "AKILL:ADD: \2%s@%s\2 (reason: \2%s\2) (duration: \2%s\2)", k->user, k->host, k->reason, timediff(k->duration));
	else
		logcommand(si, CMDLOG_ADMIN, "AKILL:ADD: \2%s@%s\2 (reason: \2%s\2) (duration: \2Permanent\2)", k->user, k->host, k->reason);
}

static void
akill_removed(struct sourceinfo *si, struct kline *k)
{
	command_success_nodata(si, _("AKILL on \2%s@%s\2 has been successfully removed."), k->user, k->host);

	verbose_wallops("\2%s\2 is \2removing\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2",
		get_oper_name(si), k->user, k->host, k->reason);

	logcommand(si, CMDLOG_ADMIN, "AKILL:DEL: \2%s@%s\2", k->user, k->host);

	kline_delete(k);
}

/*
 * Delete the AKILLs numbered FIRST to LAST. It walks the AKILL list once, so
 * it takes the same time for "1:5" as for "1:4000000000" and cannot flood.
 */
static void
akill_delete_numbers(struct sourceinfo *si, unsigned long first, unsigned long last)
{
	mowgli_node_t *n, *tn;
	unsigned int removed = 0;

	if (first > last)
	{
		command_fail(si, fault_badparams, _("Invalid range \2%lu:%lu\2: the first number is larger than the last."), first, last);
		return;
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, klnlist.head)
	{
		struct kline *k = n->data;

		if (k->number < first || k->number > last)
			continue;

		akill_removed(si, k);
		removed++;
	}

	if (removed == 0 && first == last)
		command_fail(si, fault_nosuch_target, _("No such AKILL with number \2%lu\2."), first);
	else if (removed == 0)
		command_fail(si, fault_nosuch_target, _("No AKILL with a number from \2%lu\2 to \2%lu\2."), first, last);
}

/*
 * Delete the AKILL for "user@host", or for "host" (the user is then "*").
 * The host can be anything: IPv4, IPv6, CIDR, a host name or a wildcard mask.
 */
static void
akill_delete_mask(struct sourceinfo *si, char *mask)
{
	char star[] = "*";
	char *userbuf = star, *hostbuf = mask, *at;
	struct kline *k;

	if ((at = strchr(mask, '@')) != NULL)
	{
		*at = '\0';
		hostbuf = at + 1;

		if (*mask)
			userbuf = mask;
	}

	if (!*hostbuf)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKILL DEL");
		command_fail(si, fault_needmoreparams, _("Syntax: AKILL DEL <hostmask|number|first:last>"));
		return;
	}

	if (strchr(hostbuf, '@'))
	{
		command_fail(si, fault_badparams, _("Too many '%c' in user@host."), '@');
		return;
	}

	if (!(k = kline_find(userbuf, hostbuf)))
	{
		command_fail(si, fault_nosuch_target, _("No such AKILL: \2%s@%s\2."), userbuf, hostbuf);
		return;
	}

	akill_removed(si, k);
}

static void
os_cmd_akill_del(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *token, *save = NULL;
	bool any = false;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKILL DEL");
		command_fail(si, fault_needmoreparams, _("Syntax: AKILL DEL <hostmask|number|first:last>"));
		return;
	}

	// A comma separated list of numbers, ranges and masks: "1,4:6,*@192.0.2.1"
	for (token = strtok_r(target, ",", &save); token != NULL; token = strtok_r(NULL, ",", &save))
	{
		unsigned long first, last;

		any = true;

		if (akill_parse_numbers(token, &first, &last))
			akill_delete_numbers(si, first, last);
		else
			akill_delete_mask(si, token);
	}

	if (!any)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKILL DEL");
		command_fail(si, fault_needmoreparams, _("Syntax: AKILL DEL <hostmask|number|first:last>"));
	}
}

static void
os_cmd_akill_list(struct sourceinfo *si, int parc, char *parv[])
{
	char *param = parv[0];
	char *user = NULL, *host = NULL;
	unsigned long num = 0;
	bool full = false;
	mowgli_node_t *n;
	struct kline *k;

	if (param != NULL)
	{
		if (!strcasecmp(param, "FULL"))
			full = true;
		else if ((host = strchr(param, '@')) != NULL)
		{
			*host++ = '\0';
			user = param;
			full = true;
		}
		else if (strchr(param, '.') || strchr(param, ':'))
		{
			host = param;
			full = true;
		}
		else if (isdigit((unsigned char)param[0]) &&
				(num = strtoul(param, NULL, 10)) != 0)
			full = true;
		else
		{
			command_fail(si, fault_badparams, STR_INVALID_PARAMS, "AKILL LIST");
			return;
		}
	}

	if (user || host || num)
		command_success_nodata(si, _("AKILL list matching given criteria (with reasons):"));
	else if (full)
		command_success_nodata(si, _("AKILL list (with reasons):"));
	else
		command_success_nodata(si, _("AKILL list:"));

	MOWGLI_ITER_FOREACH(n, klnlist.head)
	{
		struct tm *tm;
		char settime[64];

		k = (struct kline *)n->data;

		tm = localtime(&k->settime);

		if (tm == NULL || strftime(settime, sizeof settime, TIME_FORMAT, tm) == 0)
			mowgli_strlcpy(settime, "?", sizeof settime);

		if (num != 0 && k->number != num)
			continue;
		if (user != NULL && match(k->user, user))
			continue;
		if (host != NULL && match(k->host, host) && match_ips(k->host, host))
			continue;

		if (k->duration && full)
			command_success_nodata(si, _("%lu: %s@%s - by \2%s\2 on %s - expires in \2%s\2 - (%s)"), k->number, k->user, k->host, k->setby, settime, timediff(k->expires > CURRTIME ? k->expires - CURRTIME : 0), k->reason);
		else if (k->duration && !full)
			command_success_nodata(si, _("%lu: %s@%s - by \2%s\2 on %s - expires in \2%s\2"), k->number, k->user, k->host, k->setby, settime, timediff(k->expires > CURRTIME ? k->expires - CURRTIME : 0));
		else if (!k->duration && full)
			command_success_nodata(si, _("%lu: %s@%s - by \2%s\2 on %s - \2permanent\2 - (%s)"), k->number, k->user, k->host, k->setby, settime, k->reason);
		else
			command_success_nodata(si, _("%lu: %s@%s - by \2%s\2 on %s - \2permanent\2"), k->number, k->user, k->host, k->setby, settime);
	}

	if (user || host || num)
		command_success_nodata(si, _("End of AKILL list."));
	else
		command_success_nodata(si, ngettext(N_("Total of \2%zu\2 entry in AKILL list."),
		                                    N_("Total of \2%zu\2 entries in AKILL list."),
		                                    klnlist.count), klnlist.count);
	if (user || host)
		logcommand(si, CMDLOG_GET, "AKILL:LIST: \2%s@%s\2", user ? user : "*", host ? host : "*");
	else if (num)
		logcommand(si, CMDLOG_GET, "AKILL:LIST: \2%lu\2", num);
	else
		logcommand(si, CMDLOG_GET, "AKILL:LIST: \2%s\2", full ? " FULL" : "");
}

static void
os_cmd_akill_sync(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	struct kline *k;

	logcommand(si, CMDLOG_DO, "AKILL:SYNC");

	MOWGLI_ITER_FOREACH(n, klnlist.head)
	{
		k = (struct kline *)n->data;


		char reason[BUFSIZE];
		snprintf(reason, sizeof(reason), "[#%lu] %s", k->number, k->reason);

		if (k->duration == 0)
			kline_sts("*", k->user, k->host, 0, reason);
		else if (k->expires > CURRTIME)
			kline_sts("*", k->user, k->host, k->expires - CURRTIME, reason);
	}

	command_success_nodata(si, _("AKILL list synchronized to servers."));
}

static struct command os_akill = {
	.name           = "AKILL",
	.desc           = N_("Manages network host bans."),
	.access         = PRIV_AKILL,
	.maxparc        = 3,
	.cmd            = &os_cmd_akill,
	.help           = { .path = "oservice/akill" },
};

static struct command os_akill_add = {
	.name           = "ADD",
	.desc           = N_("Adds a network host ban."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &os_cmd_akill_add,
	.help           = { .path = "" },
};

static struct command os_akill_del = {
	.name           = "DEL",
	.desc           = N_("Deletes a network host ban."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_akill_del,
	.help           = { .path = "" },
};

static struct command os_akill_list = {
	.name           = "LIST",
	.desc           = N_("Lists all network host bans."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_akill_list,
	.help           = { .path = "" },
};

static struct command os_akill_sync = {
	.name           = "SYNC",
	.desc           = N_("Synchronises network host bans to all servers."),
	.access         = AC_NONE,
	.maxparc        = 0,
	.cmd            = &os_cmd_akill_sync,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	if (! (os_akill_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&os_akill_add, os_akill_cmds);
	(void) command_add(&os_akill_del, os_akill_cmds);
	(void) command_add(&os_akill_list, os_akill_cmds);
	(void) command_add(&os_akill_sync, os_akill_cmds);

	(void) service_named_bind_command("operserv", &os_akill);

	(void) hook_add_user_add(&os_akill_newuser);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) hook_del_user_add(&os_akill_newuser);

	(void) service_named_unbind_command("operserv", &os_akill);

	(void) mowgli_patricia_destroy(os_akill_cmds, NULL, NULL);
}

SIMPLE_DECLARE_MODULE_V1("operserv/akill", MODULE_UNLOAD_CAPABILITY_OK)
