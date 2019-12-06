/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2007 Atheme Project (http://atheme.org/)
 *
 * This file contains functionality implementing clone detection.
 */

#include <atheme.h>

#define CLONESDB_VERSION	3
#define CLONES_GRACE_TIMEPERIOD	180

struct clones_exemption
{
	char *ip;
	unsigned int allowed;
	unsigned int warn;
	char *reason;
	long expires;
};

struct clones_hostentry
{
	char ip[HOSTIPLEN + 1];
	mowgli_list_t clients;
	time_t firstkill;
	unsigned int gracekills;
};

static mowgli_patricia_t *os_clones_cmds = NULL;
static mowgli_patricia_t *hostlist = NULL;
static mowgli_heap_t *hostentry_heap = NULL;
static struct service *serviceinfo = NULL;

static mowgli_list_t clone_exempts;
static bool kline_enabled;
static unsigned int grace_count;
static long kline_duration = SECONDS_PER_HOUR;
static unsigned int clones_allowed, clones_warn;
static unsigned int clones_dbversion = 1;

static inline bool
cexempt_expired(struct clones_exemption *c)
{
	if (c && c->expires && CURRTIME > c->expires)
		return true;

	return false;
}

static void
clones_configready(void *unused)
{
	clones_allowed = config_options.default_clone_allowed;
	clones_warn = config_options.default_clone_warn;
}

static void
write_exemptdb(struct database_handle *db)
{
	mowgli_node_t *n, *tn;

	db_start_row(db,"CLONES-DBV");
	db_write_uint(db, CLONESDB_VERSION);
	db_commit_row(db);

	db_start_row(db, "CLONES-CK");
	db_write_uint(db, kline_enabled);
	db_commit_row(db);
	db_start_row(db, "CLONES-CD");
	db_write_uint(db, kline_duration);
	db_commit_row(db);
	db_start_row(db, "CLONES-GR");
	db_write_uint(db, grace_count);
	db_commit_row(db);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, clone_exempts.head)
	{
		struct clones_exemption *c = n->data;
		if (cexempt_expired(c))
		{
			sfree(c->ip);
			sfree(c->reason);
			sfree(c);
			mowgli_node_delete(n, &clone_exempts);
			mowgli_node_free(n);
		}
		else
		{
			db_start_row(db, "CLONES-EX");
			db_write_word(db, c->ip);
			db_write_uint(db, c->allowed);
			db_write_uint(db, c->warn);
			db_write_time(db, c->expires);
			db_write_str(db, c->reason);
			db_commit_row(db);
		}
	}
}

static void
db_h_clonesdbv(struct database_handle *db, const char *type)
{
	clones_dbversion = db_sread_uint(db);
}

static void
db_h_ck(struct database_handle *db, const char *type)
{
	kline_enabled = db_sread_uint(db) != 0;
}

static void
db_h_cd(struct database_handle *db, const char *type)
{
	kline_duration = db_sread_uint(db);
}

static void
db_h_gr(struct database_handle *db, const char *type)
{
	grace_count = db_sread_uint(db);
}

static void
db_h_ex(struct database_handle *db, const char *type)
{
	unsigned int allowed, warn;

	const char *ip = db_sread_word(db);
	allowed = db_sread_uint(db);

	if (clones_dbversion == 3)
	{
		warn = db_sread_uint(db);
	}
	else if (clones_dbversion == 2)
	{
		warn = db_sread_uint(db);
		db_sread_uint(db); // trash the old KILL value
	}
	else
	{
		warn = allowed;
	}

	time_t expires = db_sread_time(db);
	const char *reason = db_sread_str(db);

	struct clones_exemption *const c = smalloc(sizeof *c);
	c->ip = sstrdup(ip);
	c->allowed = allowed;
	c->warn = warn;
	c->expires = expires;
	c->reason = sstrdup(reason);
	mowgli_node_add(c, mowgli_node_create(), &clone_exempts);
}

static struct clones_exemption *
find_exempt(const char *ip)
{
	mowgli_node_t *n;

	// first check for an exact match
	MOWGLI_ITER_FOREACH(n, clone_exempts.head)
	{
		struct clones_exemption *c = n->data;

		if (!strcmp(ip, c->ip))
			return c;
	}

	// then look for cidr
	MOWGLI_ITER_FOREACH(n, clone_exempts.head)
	{
		struct clones_exemption *c = n->data;

		if (!match_ips(c->ip, ip))
			return c;
	}

	return 0;
}

static void
os_cmd_clones(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES");
		command_fail(si, fault_needmoreparams, _("Syntax: CLONES KLINE|LIST|ADDEXEMPT|DELEXEMPT|LISTEXEMPT|SETEXEMPT|DURATION [parameters]"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, os_clones_cmds, "CLONES");
}

static void
os_cmd_clones_kline(struct sourceinfo *si, int parc, char *parv[])
{
	const char *arg = parv[0];

	if (arg == NULL)
		arg = "";

	if (!strcasecmp(arg, "ON"))
	{
		if (kline_enabled && grace_count == 0)
		{
			command_fail(si, fault_nochange, _("CLONES klines are already enabled."));
			return;
		}
		kline_enabled = true;
		grace_count = 0;
		command_success_nodata(si, _("Enabled CLONES klines."));
		wallops("\2%s\2 enabled CLONES klines", get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "CLONES:KLINE:ON");
	}
	else if (!strcasecmp(arg, "OFF"))
	{
		if (!kline_enabled)
		{
			command_fail(si, fault_nochange, _("CLONES klines are already disabled."));
			return;
		}
		kline_enabled = false;
		command_success_nodata(si, _("Disabled CLONES klines."));
		wallops("\2%s\2 disabled CLONES klines", get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "CLONES:KLINE:OFF");
	}
	else if (isdigit((unsigned char)arg[0]))
	{
		unsigned int newgrace = atol(arg);
		if (kline_enabled && grace_count == newgrace)
		{
			command_fail(si, fault_nochange, _("CLONES kline grace is already enabled and set to \2%u\2 kills."), grace_count);
		}
		kline_enabled = true;
		grace_count = newgrace;
		command_success_nodata(si, ngettext(N_("Enabled CLONES klines with a grace of \2%u\2 kill"),
		                                    N_("Enabled CLONES klines with a grace of \2%u\2 kills"),
		                                    grace_count), grace_count);
		wallops("\2%s\2 enabled CLONES klines with a grace of %u kills", get_oper_name(si), grace_count);
		logcommand(si, CMDLOG_ADMIN, "CLONES:KLINE:ON grace %u", grace_count);
	}
	else
	{
		if (kline_enabled)
		{
			if (grace_count)
				command_success_string(si, "ON", ngettext(N_("CLONES klines are currently enabled with a grace of \2%u\2 kill."),
				                                          N_("CLONES klines are currently enabled with a grace of \2%u\2 kills."),
				                                          grace_count), grace_count);
			else
				command_success_string(si, "ON", _("CLONES klines are currently enabled."));
		}
		else
			command_success_string(si, "OFF", _("CLONES klines are currently disabled."));
	}
}

static void
os_cmd_clones_list(struct sourceinfo *si, int parc, char *parv[])
{
	struct clones_hostentry *he;
	unsigned int k = 0;
	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(he, &state, hostlist)
	{
		k = MOWGLI_LIST_LENGTH(&he->clients);

		if (k > 3)
		{
			struct clones_exemption *c = find_exempt(he->ip);
			if (c)
				command_success_nodata(si, _("%u from %s (\2EXEMPT\2; allowed %u)"), k, he->ip, c->allowed);
			else
				command_success_nodata(si, _("%u from %s"), k, he->ip);
		}
	}
	command_success_nodata(si, _("End of CLONES LIST"));
	logcommand(si, CMDLOG_ADMIN, "CLONES:LIST");
}

static void
os_cmd_clones_addexempt(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	char *ip = parv[0];
	char *clonesstr = parv[1];
	unsigned int clones;
	char *expiry = parv[2];
	char *reason = parv[3];
	char rreason[BUFSIZE];
	struct clones_exemption *c = NULL;
	long duration;

	if (!ip || !clonesstr || !expiry || ! string_to_uint(clonesstr, &clones) || ! clones)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES ADDEXEMPT");
		command_fail(si, fault_needmoreparams, _("Syntax: CLONES ADDEXEMPT <ip> <clones> [!P|!T <minutes>] <reason>"));
		return;
	}

	if (!valid_ip_or_mask(ip))
	{
		command_fail(si, fault_badparams, _("Invalid IP/mask given."));
		command_fail(si, fault_badparams, _("Syntax: CLONES ADDEXEMPT <ip> <clones> [!P|!T <minutes>] <reason>"));
		return;
	}

	if (expiry && !strcasecmp(expiry, "!P"))
	{
		if (!reason)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES ADDEXEMPT");
			command_fail(si, fault_needmoreparams, _("Syntax: CLONES ADDEXEMPT <ip> <clones> [!P|!T <minutes>] <reason>"));
			return;
		}

		duration = 0;
		mowgli_strlcpy(rreason, reason, BUFSIZE);
	}
	else if (expiry && !strcasecmp(expiry, "!T"))
	{
		if (!reason)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES ADDEXEMPT");
			command_fail(si, fault_needmoreparams, _("Syntax: CLONES ADDEXEMPT <ip> <clones> [!P|!T <minutes>] <reason>"));
			return;
		}

		reason = strchr(reason, ' ');
		if (reason)
			*reason++ = '\0';
		expiry += 3;

		duration = (atol(expiry) * SECONDS_PER_MINUTE);
		while (isdigit((unsigned char)*expiry))
			++expiry;
		if (*expiry == 'h' || *expiry == 'H')
			duration *= MINUTES_PER_HOUR;
		else if (*expiry == 'd' || *expiry == 'D')
			duration *= MINUTES_PER_DAY;
		else if (*expiry == 'w' || *expiry == 'W')
			duration *= MINUTES_PER_WEEK;
		else if (*expiry == '\0')
			;
		else
			duration = 0;

		if (duration == 0)
		{
			command_fail(si, fault_badparams, _("Invalid duration given."));
			command_fail(si, fault_badparams, _("Syntax: CLONES ADDEXEMPT <ip> <clones> [!P|!T <minutes>] <reason>"));
			return;
		}

		if (!reason)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES ADDEXEMPT");
			command_fail(si, fault_needmoreparams, _("Syntax: CLONES ADDEXEMPT <ip> <clones> [!P|!T <minutes>] <reason>"));
			return;
		}

		mowgli_strlcpy(rreason, reason, BUFSIZE);
	}
	else if (expiry)
	{
		duration = config_options.clone_time;

		mowgli_strlcpy(rreason, expiry, BUFSIZE);
		if (reason)
		{
			mowgli_strlcat(rreason, " ", BUFSIZE);
			mowgli_strlcat(rreason, reason, BUFSIZE);
		}
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES ADDEXEMPT");
		command_fail(si, fault_needmoreparams, _("Syntax: CLONES ADDEXEMPT <ip> <clones> [!P|!T <minutes>] <reason>"));
		return;
	}

	MOWGLI_ITER_FOREACH(n, clone_exempts.head)
	{
		struct clones_exemption *t = n->data;

		if (!strcmp(ip, t->ip))
			c = t;
	}

	if (c == NULL)
	{
		if (!*rreason)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES ADDEXEMPT");
			command_fail(si, fault_needmoreparams, _("Syntax: CLONES ADDEXEMPT <ip> <clones> [!P|!T <minutes>] <reason>"));
			return;
		}

		c = smalloc(sizeof *c);
		c->ip = sstrdup(ip);
		c->reason = sstrdup(rreason);
		mowgli_node_add(c, mowgli_node_create(), &clone_exempts);
		command_success_nodata(si, _("Added \2%s\2 to clone exempt list."), ip);
	}
	else
	{
		if (*rreason)
		{
			sfree(c->reason);
			c->reason = sstrdup(rreason);
		}
		command_success_nodata(si, _("\2Warning\2: the syntax you are using to update this exemption has been deprecated and may be removed in a future version. Please use SETEXEMPT in the future instead."));
		command_success_nodata(si, _("Updated \2%s\2 in clone exempt list."), ip);
	}

	c->allowed = clones;
	c->warn = clones;
	c->expires = duration ? (CURRTIME + duration) : 0;

	logcommand(si, CMDLOG_ADMIN, "CLONES:ADDEXEMPT: \2%s\2 \2%u\2 (reason: \2%s\2) (duration: \2%s\2)", ip, clones, c->reason, timediff(duration));
}

static void
os_cmd_clones_delexempt(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n, *tn;
	char *arg = parv[0];

	if (!arg)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES DELEXEMPT");
		command_fail(si, fault_needmoreparams, _("Syntax: CLONES DELEXEMPT <ip>"));
		return;
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, clone_exempts.head)
	{
		struct clones_exemption *c = n->data;

		if (cexempt_expired(c))
		{
			sfree(c->ip);
			sfree(c->reason);
			sfree(c);
			mowgli_node_delete(n, &clone_exempts);
			mowgli_node_free(n);
		}
		else if (!strcmp(c->ip, arg))
		{
			sfree(c->ip);
			sfree(c->reason);
			sfree(c);
			mowgli_node_delete(n, &clone_exempts);
			mowgli_node_free(n);
			command_success_nodata(si, _("Removed \2%s\2 from clone exempt list."), arg);
			logcommand(si, CMDLOG_ADMIN, "CLONES:DELEXEMPT: \2%s\2", arg);
			return;
		}
	}

	command_fail(si, fault_nosuch_target, _("\2%s\2 not found in clone exempt list."), arg);
}

static void
os_cmd_clones_setexempt(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n, *tn;
	char *ip = parv[0];
	char *subcmd = parv[1];
	char *clonesstr = parv[2];
	char *reason = parv[3];
	char rreason[BUFSIZE];

	long duration;


	if (!ip || !subcmd || !clonesstr)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES SETEXEMPT");
		command_fail(si, fault_needmoreparams, _("Syntax: CLONES SETEXEMPT [DEFAULT | <ip>] <ALLOWED | WARN> <limit>"));
		command_fail(si, fault_needmoreparams, _("Syntax: CLONES SETEXEMPT <ip> <REASON | DURATION> <value>"));
		return;
	}

	int clones = atoi(clonesstr);

	if (!strcasecmp(ip, "DEFAULT"))
	{
		if (!strcasecmp(subcmd, "ALLOWED"))
		{
			clones_allowed = clones;
			command_success_nodata(si, _("Default allowed clone limit set to \2%u\2."), clones_allowed);
		}
		else if (!strcasecmp(subcmd,"WARN"))
		{
			if (clones == 0)
			{
				command_success_nodata(si, _("Default clone warning has been disabled."));
				clones_warn = 0;
				return;
			}

			clones_warn = clones;
			command_success_nodata(si, _("Default warned clone limit set to \2%u\2"), clones_warn);
		}
		else
		{
			// Invalid parameters
			command_fail(si, fault_badparams, _("Invalid syntax given."));
			command_fail(si, fault_badparams, _("Syntax: CLONES SETEXEMPT DEFAULT <ALLOWED | WARN> <limit>"));
			return;
		}

		logcommand(si, CMDLOG_ADMIN, "CLONES:SETEXEMPT:DEFAULT: \2%s\2 \2%u\2 allowed, \2%u\2 warn", ip, clones_allowed, clones_warn);
	}
	else if (ip) {
		MOWGLI_ITER_FOREACH_SAFE(n, tn, clone_exempts.head)
		{
			struct clones_exemption *c = n->data;

			if (cexempt_expired(c))
			{
				sfree(c->ip);
				sfree(c->reason);
				sfree(c);
				mowgli_node_delete(n, &clone_exempts);
				mowgli_node_free(n);
			}
			else if (!strcmp(c->ip, ip))
			{
				if (!strcasecmp(subcmd, "ALLOWED"))
				{
					if (clones < c->warn)
					{
						command_fail(si, fault_badparams, _("Allowed clones limit must be greater than or equal to the warned limit of \2%u\2"), c->warn);
						return;
					}

					c->allowed = clones;
					command_success_nodata(si, _("Allowed clones limit for host \2%s\2 set to \2%u\2"), ip, c->allowed);
				}
				else if (!strcasecmp(subcmd, "WARN"))
				{
					if (clones == 0)
					{
						command_success_nodata(si, _("Clone warning messages will be disabled for host \2%s\2"), ip);
						c->warn = 0;
						return;
					}
					else if (clones > c->allowed)
					{
						command_fail(si, fault_badparams, _("Warned clones limit must be lower than or equal to the allowed limit of %u"), c->allowed);
						return;
					}

					c->warn = clones;
					command_success_nodata(si, _("Warned clones limit for host \2%s\2 set to \2%u\2"), ip, c->warn);
				}
				else if (!strcasecmp(subcmd, "DURATION"))
				{
					char *expiry = clonesstr;
					if (!strcmp(expiry, "0"))
					{
						c->expires = 0;
						command_success_nodata(si, _("Clone exemption duration for host \2%s\2 set to \2permanent\2"), ip);
					}
					else
					{
						duration = (atol(expiry) * SECONDS_PER_MINUTE);
						while (isdigit((unsigned char)*expiry))
							++expiry;
						if (*expiry == 'h' || *expiry == 'H')
							duration *= MINUTES_PER_HOUR;
						else if (*expiry == 'd' || *expiry == 'D')
							duration *= MINUTES_PER_DAY;
						else if (*expiry == 'w' || *expiry == 'W')
							duration *= MINUTES_PER_WEEK;
						else if (*expiry == '\0')
							;
						else
							duration = 0;

						if (duration == 0)
						{
							command_fail(si, fault_badparams, _("Invalid duration given."));
							command_fail(si, fault_needmoreparams, _("Syntax: CLONES SETEXEMPT <ip> DURATION <value>"));
							return;
						}
						c->expires = CURRTIME + duration;
						command_success_nodata(si, _("Clone exemption duration for host \2%s\2 set to \2%s\2 (%ld seconds)"), ip, clonesstr, duration);
					}

				}
				else if (!strcasecmp(subcmd, "REASON"))
				{
					mowgli_strlcpy(rreason, clonesstr, BUFSIZE);
					if (reason)
					{
						mowgli_strlcat(rreason, " ", BUFSIZE);
						mowgli_strlcat(rreason, reason, BUFSIZE);
					}

					sfree(c->reason);
					c->reason = sstrdup(rreason);
					command_success_nodata(si, _("Clone exemption reason for host \2%s\2 changed to \2%s\2"), ip, c->reason);
				}
				else
				{
					// Invalid parameters
					command_fail(si, fault_badparams, _("Invalid syntax given."));
					command_fail(si, fault_badparams, _("Syntax: CLONES SETEXEMPT <IP> <ALLOWED | WARN | DURATION | REASON> <value>"));
					return;
				}

				logcommand(si, CMDLOG_ADMIN, "CLONES:SETEXEMPT: \2%s\2 \2%d\2 (reason: \2%s\2) (duration: \2%s\2)", ip, clones, c->reason, timediff((c->expires - CURRTIME)));

				return;
			}
		}

		command_fail(si, fault_nosuch_target, _("\2%s\2 not found in clone exempt list."), ip);
	}
}

static void
os_cmd_clones_duration(struct sourceinfo *si, int parc, char *parv[])
{
	char *s = parv[0];
	long duration;

	if (!s)
	{
		command_success_nodata(si, _("Clone ban duration set to \2%ld\2 (%ld seconds)"), kline_duration / SECONDS_PER_MINUTE, kline_duration);
		return;
	}

	duration = (atol(s) * SECONDS_PER_MINUTE);
	while (isdigit((unsigned char)*s))
		s++;
	if (*s == 'h' || *s == 'H')
		duration *= MINUTES_PER_HOUR;
	else if (*s == 'd' || *s == 'D')
		duration *= MINUTES_PER_DAY;
	else if (*s == 'w' || *s == 'W')
		duration *= MINUTES_PER_WEEK;
	else if (*s == '\0' || *s == 'm' || *s == 'M')
		;
	else
		duration = 0;

	if (duration <= 0)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "CLONES DURATION");
		return;
	}

	kline_duration = duration;
	command_success_nodata(si, _("Clone ban duration set to \2%s\2 (%ld seconds)"), parv[0], kline_duration);
}

static void
os_cmd_clones_listexempt(struct sourceinfo *si, int parc, char *parv[])
{

	command_success_nodata(si, _("DEFAULT - allowed limit %u, warn on %u"), clones_allowed, clones_warn);
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, clone_exempts.head)
	{
		struct clones_exemption *c = n->data;

		if (cexempt_expired(c))
		{
			sfree(c->ip);
			sfree(c->reason);
			sfree(c);
			mowgli_node_delete(n, &clone_exempts);
			mowgli_node_free(n);
		}
		else if (c->expires)
			command_success_nodata(si, _("%s - allowed limit %u, warn on %u - expires in %s - \2%s\2"), c->ip, c->allowed, c->warn, timediff(c->expires > CURRTIME ? c->expires - CURRTIME : 0), c->reason);
		else
			command_success_nodata(si, _("%s - allowed limit %u, warn on %u - \2permanent\2 - \2%s\2"), c->ip, c->allowed, c->warn, c->reason);
	}
	command_success_nodata(si, _("End of CLONES LISTEXEMPT"));
	logcommand(si, CMDLOG_ADMIN, "CLONES:LISTEXEMPT");
}

static void
clones_newuser(struct hook_user_nick *data)
{
	struct user *u = data->u;
	unsigned int i;
	struct clones_hostentry *he;
	unsigned int allowed, warn;
	mowgli_node_t *n;

	// If the user has been killed, don't do anything.
	if (!u)
		return;

	// User has no IP, ignore them
	if (is_internal_client(u) || u->ip == NULL)
		return;

	he = mowgli_patricia_retrieve(hostlist, u->ip);
	if (he == NULL)
	{
		he = mowgli_heap_alloc(hostentry_heap);
		mowgli_strlcpy(he->ip, u->ip, sizeof he->ip);
		mowgli_patricia_add(hostlist, he->ip, he);
	}
	mowgli_node_add(u, mowgli_node_create(), &he->clients);
	i = MOWGLI_LIST_LENGTH(&he->clients);

	struct clones_exemption *c = find_exempt(u->ip);
	if (c == 0)
	{
		allowed = clones_allowed;
		warn = clones_warn;
	}
	else
	{
		allowed = c->allowed;
		warn = c->warn;
	}

	if (config_options.clone_increase)
	{
		unsigned int real_allowed = allowed;
		unsigned int real_warn = warn;

		MOWGLI_ITER_FOREACH(n, he->clients.head)
		{
			struct user *tu = n->data;

			if (tu->myuser == NULL)
				continue;
			if (allowed != 0)
				allowed++;
			if (warn != 0)
				warn++;
		}

		// A hard limit of 2x the "real" limit sounds good IMO --jdhore
		if (allowed > (real_allowed * 2))
			allowed = real_allowed * 2;
		if (warn > (real_warn * 2))
			warn = real_warn * 2;
	}

	if (i > allowed && allowed != 0)
	{
		// User has exceeded the maximum number of allowed clones.
		if (is_autokline_exempt(u))
			slog(LG_INFO, "CLONES: \2%u\2 clones on \2%s\2 (%s!%s@%s) (user is autokline exempt)", i, u->ip, u->nick, u->user, u->host);
		else if (!kline_enabled || he->gracekills < grace_count || (grace_count > 0 && he->firstkill < time(NULL) - CLONES_GRACE_TIMEPERIOD))
		{
			if (he->firstkill < time(NULL) - CLONES_GRACE_TIMEPERIOD)
			{
				he->firstkill = time(NULL);
				he->gracekills = 1;
			}
			else
			{
				he->gracekills++;
			}

			if (!kline_enabled)
				slog(LG_INFO, "CLONES: \2%u\2 clones on \2%s\2 (%s!%s@%s) (TKLINE disabled, killing user)", i, u->ip, u->nick, u->user, u->host);
			else
				slog(LG_INFO, "CLONES: \2%u\2 clones on \2%s\2 (%s!%s@%s) (grace period, killing user, %u grace kills remaining)", i, u->ip, u->nick,
					u->user, u->host, grace_count - he->gracekills);

			kill_user(serviceinfo->me, u, "Too many connections from this host.");
			data->u = NULL; // Required due to kill_user being called during user_add hook. --mr_flea
		}
		else
		{
			if (! (u->flags & UF_KLINESENT)) {
				slog(LG_INFO, "CLONES: \2%u\2 clones on \2%s\2 (%s!%s@%s) (TKLINE due to excess clones)", i, u->ip, u->nick, u->user, u->host);
				kline_sts("*", "*", u->ip, kline_duration, "Excessive clones");
				u->flags |= UF_KLINESENT;
			}
		}

	}
	else if (i >= warn && warn != 0)
	{
		slog(LG_INFO, "CLONES: \2%u\2 clones on \2%s\2 (%s!%s@%s) (\2%u\2 allowed)", i, u->ip, u->nick, u->user, u->host, allowed);
		msg(serviceinfo->nick, u->nick, _("\2WARNING\2: You may not have more than \2%u\2 clients connected to the network at once. Any further connections risks being removed."), allowed);
	}
}

static void
clones_userquit(struct user *u)
{
	mowgli_node_t *n;
	struct clones_hostentry *he;

	// User has no IP, ignore them
	if (is_internal_client(u) || u->ip == NULL)
		return;

	he = mowgli_patricia_retrieve(hostlist, u->ip);
	if (he == NULL)
	{
		slog(LG_DEBUG, "clones_userquit(): hostentry for %s not found??", u->ip);
		return;
	}
	n = mowgli_node_find(u, &he->clients);
	if (n)
	{
		mowgli_node_delete(n, &he->clients);
		mowgli_node_free(n);
		if (MOWGLI_LIST_LENGTH(&he->clients) == 0)
		{
			// TODO: free later if he->firstkill > time(NULL) - CLONES_GRACE_TIMEPERIOD.
			mowgli_patricia_delete(hostlist, he->ip);
			mowgli_heap_free(hostentry_heap, he);
		}
	}
}

static struct command os_clones = {
	.name           = "CLONES",
	.desc           = N_("Manages network wide clones."),
	.access         = PRIV_AKILL,
	.maxparc        = 5,
	.cmd            = &os_cmd_clones,
	.help           = { .path = "oservice/clones" },
};

static struct command os_clones_kline = {
	.name           = "KLINE",
	.desc           = N_("Enables/disables klines for excessive clones."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_clones_kline,
	.help           = { .path = "" },
};

static struct command os_clones_list = {
	.name           = "LIST",
	.desc           = N_("Lists clones on the network."),
	.access         = AC_NONE,
	.maxparc        = 0,
	.cmd            = &os_cmd_clones_list,
	.help           = { .path = "" },
};

static struct command os_clones_addexempt = {
	.name           = "ADDEXEMPT",
	.desc           = N_("Adds a clones exemption."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &os_cmd_clones_addexempt,
	.help           = { .path = "" },
};

static struct command os_clones_delexempt = {
	.name           = "DELEXEMPT",
	.desc           = N_("Deletes a clones exemption."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_clones_delexempt,
	.help           = { .path = "" },
};

static struct command os_clones_setexempt = {
	.name           = "SETEXEMPT",
	.desc           = N_("Sets a clone exemption details."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_clones_setexempt,
	.help           = { .path = "" },
};

static struct command os_clones_listexempt = {
	.name           = "LISTEXEMPT",
	.desc           = N_("Lists clones exemptions."),
	.access         = AC_NONE,
	.maxparc        = 0,
	.cmd            = &os_cmd_clones_listexempt,
	.help           = { .path = "" },
};

static struct command os_clones_duration = {
	.name           = "DURATION",
	.desc           = N_("Sets a custom duration to ban clones for."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_clones_duration,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	if (! module_find_published("backend/opensex"))
	{
		(void) slog(LG_ERROR, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	if (! (serviceinfo = service_find("operserv")))
	{
		(void) slog(LG_ERROR, "%s: cannot find OperServ (BUG?)", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	if (! (os_clones_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	if (! (hostlist = mowgli_patricia_create(&noopcanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		(void) mowgli_patricia_destroy(os_clones_cmds, NULL, NULL);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	if (! (hostentry_heap = mowgli_heap_create(sizeof(struct clones_hostentry), HEAP_USER, BH_NOW)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_heap_create() failed", m->name);

		(void) mowgli_patricia_destroy(os_clones_cmds, NULL, NULL);
		(void) mowgli_patricia_destroy(hostlist, NULL, NULL);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&os_clones_kline, os_clones_cmds);
	(void) command_add(&os_clones_list, os_clones_cmds);
	(void) command_add(&os_clones_addexempt, os_clones_cmds);
	(void) command_add(&os_clones_delexempt, os_clones_cmds);
	(void) command_add(&os_clones_setexempt, os_clones_cmds);
	(void) command_add(&os_clones_listexempt, os_clones_cmds);
	(void) command_add(&os_clones_duration, os_clones_cmds);

	(void) service_named_bind_command("operserv", &os_clones);

	(void) hook_add_config_ready(&clones_configready);
	(void) hook_add_user_add(&clones_newuser);
	(void) hook_add_user_delete(&clones_userquit);
	(void) hook_add_db_write(&write_exemptdb);

	(void) db_register_type_handler("CLONES-DBV", &db_h_clonesdbv);
	(void) db_register_type_handler("CLONES-CK", &db_h_ck);
	(void) db_register_type_handler("CLONES-CD", &db_h_cd);
	(void) db_register_type_handler("CLONES-GR", &db_h_gr);
	(void) db_register_type_handler("CLONES-EX", &db_h_ex);

	// add everyone to host hash
	struct user *u;
	mowgli_patricia_iteration_state_t state;
	MOWGLI_PATRICIA_FOREACH(u, &state, userlist)
		(void) clones_newuser(&(struct hook_user_nick){ .u = u });

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("operserv/clones", MODULE_UNLOAD_CAPABILITY_NEVER)
