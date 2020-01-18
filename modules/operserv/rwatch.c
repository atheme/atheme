/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006 Atheme Project (http://atheme.org/)
 *
 * This file contains functionality implementing OperServ RWATCH.
 */

#include <atheme.h>

#define RWACT_SNOOP 		1
#define RWACT_KLINE 		2
#define RWACT_QUARANTINE	4

struct rwatch
{
	char *regex;
	int reflags; // AREGEX_*
	char *reason;
	int actions; // RWACT_*
	struct atheme_regex *re;
};

static struct rwatch *rwread = NULL;
static FILE *f;

static mowgli_patricia_t *os_rwatch_cmds;
static mowgli_list_t rwatch_list;

static void
write_rwatchdb(struct database_handle *db)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, rwatch_list.head)
	{
		struct rwatch *rw = n->data;

		db_start_row(db, "RW");
		db_write_uint(db, rw->reflags);
		db_write_str(db, rw->regex);
		db_commit_row(db);

		db_start_row(db, "RR");
		db_write_uint(db, rw->actions);
		db_write_str(db, rw->reason);
		db_commit_row(db);
	}
}

static void
load_rwatchdb(char *path)
{
	char *item, rBuf[BUFSIZE * 2];
	struct rwatch *rw = NULL;
	char newpath[BUFSIZE];

	snprintf(newpath, BUFSIZE, "%s/%s", datadir, "rwatch.db.old");

	while (fgets(rBuf, BUFSIZE * 2, f))
	{
		item = strtok(rBuf, " ");
		if (!item)
			continue;

		strip(item);

		if (!strcmp(item, "RW"))
		{
			char *reflagsstr = strtok(NULL, " ");
			char *regex = strtok(NULL, "\n");

			if (!reflagsstr || !regex || rw)
				; // erroneous, don't add
			else
			{
				rw = smalloc(sizeof *rw);
				rw->regex = sstrdup(regex);
				rw->reflags = atoi(reflagsstr);
				rw->re = regex_create(rw->regex, rw->reflags);
			}
		}
		else if (!strcmp(item, "RR"))
		{
			char *actionstr = strtok(NULL, " ");
			char *reason = strtok(NULL, "\n");

			if (!actionstr || !reason || !rw)
				; // erroneous, don't add
			else
			{
				rw->actions = atoi(actionstr);
				rw->reason = sstrdup(reason);
				mowgli_node_add(rw, mowgli_node_create(), &rwatch_list);
				rw = NULL;
			}
		}
	}

	fclose(f);

	if ((srename(path, newpath)) < 0)
	{
		slog(LG_ERROR, "load_rwatchdb(): couldn't rename rwatch database.");
	}
	else
	{
		slog(LG_INFO, "The RWATCH database has been converted to the OpenSEX format.");
		slog(LG_INFO, "The old RWATCH database now resides in rwatch.db.old which may be deleted.");
	}

	if (rw != NULL)
	{
		sfree(rw->regex);
		sfree(rw->reason);
		if (rw->re != NULL)
			regex_destroy(rw->re);
		sfree(rw);
	}
}

static void
db_h_rw(struct database_handle *db, const char *type)
{
	int reflags = db_sread_uint(db);
	const char *regex = db_sread_str(db);

	rwread = smalloc(sizeof *rwread);
	rwread->regex = sstrdup(regex);
	rwread->reflags = reflags;
	rwread->re = regex_create(rwread->regex, rwread->reflags);
}

static void
db_h_rr(struct database_handle *db, const char *type)
{
	int actions = db_sread_uint(db);
	const char *reason = db_sread_str(db);

	rwread->actions = actions;
	rwread->reason = sstrdup(reason);
	mowgli_node_add(rwread, mowgli_node_create(), &rwatch_list);
	rwread = NULL;
}

static void
os_cmd_rwatch(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: RWATCH ADD|DEL|LIST|SET"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, os_rwatch_cmds, "RWATCH");
}

static void
os_cmd_rwatch_add(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	char *pattern;
	char *reason;
	struct atheme_regex *regex;
	int flags;
	char *args = parv[0];

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH ADD");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH ADD /<regex>/[i] <reason>"));
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RWATCH ADD");
		command_fail(si, fault_badparams, _("Syntax: RWATCH ADD /<regex>/[i] <reason>"));
		return;
	}

	reason = args;
	while (*reason == ' ')
		reason++;
	if (*reason == '\0')
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH ADD");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH ADD /<regex>/[i] <reason>"));
		return;
	}

	MOWGLI_ITER_FOREACH(n, rwatch_list.head)
	{
		struct rwatch *t = n->data;

		if (!strcmp(pattern, t->regex))
		{
			command_fail(si, fault_nochange, _("\2%s\2 already found in regex watch list; not adding."), pattern);
			return;
		}
	}

	regex = regex_create(pattern, flags);
	if (regex == NULL)
	{
		command_fail(si, fault_badparams, _("The provided regex \2%s\2 is invalid."), pattern);
		return;
	}

	struct rwatch *const rw = smalloc(sizeof *rw);
	rw->regex = sstrdup(pattern);
	rw->reflags = flags;
	rw->reason = sstrdup(reason);
	rw->actions = RWACT_SNOOP | ((flags & AREGEX_KLINE) == AREGEX_KLINE ? RWACT_KLINE : 0);
	rw->re = regex;

	mowgli_node_add(rw, mowgli_node_create(), &rwatch_list);
	command_success_nodata(si, _("Added \2%s\2 to regex watch list."), pattern);
	logcommand(si, CMDLOG_ADMIN, "RWATCH:ADD: \2%s\2 (reason: \2%s\2)", pattern, reason);
}

static void
os_cmd_rwatch_del(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n, *tn;
	char *pattern;
	int flags;
	char *args = parv[0];

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH DEL");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH DEL /<regex>/[i]"));
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RWATCH DEL");
		command_fail(si, fault_badparams, _("Syntax: RWATCH DEL /<regex>/[i]"));
		return;
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, rwatch_list.head)
	{
		struct rwatch *rw = n->data;

		if (!strcmp(rw->regex, pattern))
		{
			if (rw->actions & RWACT_KLINE)
			{
				if (!has_priv(si, PRIV_MASS_AKILL))
				{
					command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_MASS_AKILL);
					return;
				}
				wallops("\2%s\2 disabled kline on regex watch pattern \2%s\2", get_oper_name(si), pattern);
			}
			if (rw->actions & RWACT_QUARANTINE)
			{
				if (!has_priv(si, PRIV_MASS_AKILL))
				{
					command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_MASS_AKILL);
					return;
				}
				wallops("\2%s\2 disabled quarantine on regex watch pattern \2%s\2", get_oper_name(si), pattern);
			}
			sfree(rw->regex);
			sfree(rw->reason);
			if (rw->re != NULL)
				regex_destroy(rw->re);
			sfree(rw);
			mowgli_node_delete(n, &rwatch_list);
			mowgli_node_free(n);
			command_success_nodata(si, _("Removed \2%s\2 from regex watch list."), pattern);
			logcommand(si, CMDLOG_ADMIN, "RWATCH:DEL: \2%s\2", pattern);
			return;
		}
	}

	command_fail(si, fault_nochange, _("\2%s\2 not found in regex watch list."), pattern);
}

static void
os_cmd_rwatch_list(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, rwatch_list.head)
	{
		struct rwatch *rw = n->data;

		command_success_nodata(si, "%s (%s%s%s%s) - %s",
				rw->regex,
				rw->reflags & AREGEX_ICASE ? "i" : "",
				rw->reflags & AREGEX_PCRE ? "p" : "",
				rw->actions & RWACT_SNOOP ? "S" : "",
				rw->actions & RWACT_KLINE ? "\2K\2" : "",
				rw->reason);
	}
	command_success_nodata(si, _("End of RWATCH LIST"));
	logcommand(si, CMDLOG_GET, "RWATCH:LIST");
}

static void
os_cmd_rwatch_set(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n, *tn;
	char *pattern;
	char *opts;
	int addflags = 0, removeflags = 0;
	int flags;
	char *args = parv[0];

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH SET");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH SET /<regex>/[i] [KLINE] [NOKLINE] [SNOOP] [NOSNOOP] [QUARANTINE] [NOQUARANTINE]"));
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RWATCH SET");
		command_fail(si, fault_badparams, _("Syntax: RWATCH SET /<regex>/[i] [KLINE] [NOKLINE] [SNOOP] [NOSNOOP] [QUARANTINE] [NOQUARANTINE]"));
		return;
	}
	while (*args == ' ')
		args++;

	if (*args == '\0')
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH SET");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH SET /<regex>/[i] [KLINE] [NOKLINE] [SNOOP] [NOSNOOP] [QUARANTINE] [NOQUARANTINE]"));
		return;
	}

	opts = args;
	while (*args != '\0')
	{
		if (!strncasecmp(args, "KLINE", 5))
		{
			addflags |= RWACT_KLINE;
			removeflags &= ~RWACT_KLINE;
			args += 5;
		}
		else if (!strncasecmp(args, "NOKLINE", 7))
		{
			removeflags |= RWACT_KLINE;
			addflags &= ~RWACT_KLINE;
			args += 7;
		}
		else if (!strncasecmp(args, "SNOOP", 5))
		{
			addflags |= RWACT_SNOOP;
			removeflags &= ~RWACT_SNOOP;
			args += 5;
		}
		else if (!strncasecmp(args, "NOSNOOP", 7))
		{
			removeflags |= RWACT_SNOOP;
			addflags &= ~RWACT_SNOOP;
			args += 7;
		}
		else if (!strncasecmp(args, "QUARANTINE", 10))
		{
			addflags |= RWACT_QUARANTINE;
			removeflags &= ~RWACT_QUARANTINE;
			args += 10;
		}
		else if (!strncasecmp(args, "NOQUARANTINE", 12))
		{
			removeflags |= RWACT_QUARANTINE;
			addflags &= ~RWACT_QUARANTINE;
			args += 12;
		}

		if (*args != '\0' && *args != ' ')
		{
			command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RWATCH SET");
			command_fail(si, fault_badparams, _("Syntax: RWATCH SET /<regex>/[i] [KLINE] [NOKLINE] [SNOOP] [NOSNOOP] [QUARANTINE] [NOQUARANTINE]"));
			return;
		}
		while (*args == ' ')
			args++;
	}

	if ((addflags | removeflags) & RWACT_KLINE && !has_priv(si, PRIV_MASS_AKILL))
	{
		command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_MASS_AKILL);
		return;
	}

	if ((addflags | removeflags) & RWACT_QUARANTINE && !has_priv(si, PRIV_MASS_AKILL))
	{
		command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_MASS_AKILL);
		return;
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, rwatch_list.head)
	{
		struct rwatch *rw = n->data;

		if (!strcmp(rw->regex, pattern))
		{
			if (((~rw->actions & addflags) | (rw->actions & removeflags)) == 0)
			{
				command_fail(si, fault_nochange, _("Options for \2%s\2 unchanged."), pattern);
				return;
			}
			rw->actions |= addflags;
			rw->actions &= ~removeflags;
			command_success_nodata(si, _("Set options \2%s\2 on \2%s\2."), opts, pattern);

			if (addflags & RWACT_KLINE)
				wallops("\2%s\2 enabled kline on regex watch pattern \2%s\2", get_oper_name(si), pattern);
			if (removeflags & RWACT_KLINE)
				wallops("\2%s\2 disabled kline on regex watch pattern \2%s\2", get_oper_name(si), pattern);

			if (addflags & RWACT_QUARANTINE)
				wallops("\2%s\2 enabled quarantine on regex watch pattern \2%s\2", get_oper_name(si), pattern);
			if (removeflags & RWACT_QUARANTINE)
				wallops("\2%s\2 disabled quarantine on regex watch pattern \2%s\2", get_oper_name(si), pattern);

			logcommand(si, CMDLOG_ADMIN, "RWATCH:SET: \2%s\2 \2%s\2", pattern, opts);
			return;
		}
	}

	command_fail(si, fault_nosuch_target, _("\2%s\2 not found in regex watch list."), pattern);
}

static void
rwatch_newuser(struct hook_user_nick *data)
{
	struct user *u = data->u;
	char usermask[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1 + GECOSLEN + 1];
	mowgli_node_t *n;
	struct rwatch *rw;

	// If the user has been killed, don't do anything.
	if (!u)
		return;

	if (is_internal_client(u))
		return;

	snprintf(usermask, sizeof usermask, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);

	MOWGLI_ITER_FOREACH(n, rwatch_list.head)
	{
		rw = n->data;
		if (!rw->re)
			continue;
		if (regex_match(rw->re, usermask))
		{
			if (rw->actions & RWACT_SNOOP)
			{
				slog(LG_INFO, "RWATCH:%s \2%s\2 matches \2%s\2 (reason: \2%s\2)",
						rw->actions & RWACT_KLINE ? "KLINE:" : "",
						usermask, rw->regex, rw->reason);
			}
			if (rw->actions & RWACT_KLINE)
			{
				if (is_autokline_exempt(u))
					slog(LG_INFO, "rwatch_newuser(): not klining *@%s (user %s!%s@%s is autokline exempt but matches %s %s)",
							u->host, u->nick, u->user, u->host,
							rw->regex, rw->reason);
				else
				{
					slog(LG_VERBOSE, "rwatch_newuser(): klining *@%s (user %s!%s@%s matches %s %s)",
							u->host, u->nick, u->user, u->host,
							rw->regex, rw->reason);
					if (! (u->flags & UF_KLINESENT)) {
						kline_sts("*", "*", u->host, SECONDS_PER_DAY, rw->reason);
						u->flags |= UF_KLINESENT;
					}
				}
			}
			else if (rw->actions & RWACT_QUARANTINE)
			{
				if (is_autokline_exempt(u))
					slog(LG_INFO, "rwatch_newuser(): not qurantining *@%s (user %s!%s@%s is autokline exempt but matches %s %s)",
							u->host, u->nick, u->user, u->host,
							rw->regex, rw->reason);
				else
				{
					slog(LG_VERBOSE, "rwatch_newuser(): quaranting *@%s (user %s!%s@%s matches %s %s)",
							u->host, u->nick, u->user, u->host,
							rw->regex, rw->reason);
					quarantine_sts(service_find("operserv")->me, u, SECONDS_PER_DAY, rw->reason);
				}
			}
		}
	}
}

static void
rwatch_nickchange(struct hook_user_nick *data)
{
	struct user *u = data->u;
	char usermask[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1 + GECOSLEN + 1];
	char oldusermask[NICKLEN + 1 + USERLEN + 1 + HOSTLEN + 1 + GECOSLEN + 1];
	mowgli_node_t *n;
	struct rwatch *rw;

	// If the user has been killed, don't do anything.
	if (!u)
		return;

	if (is_internal_client(u))
		return;

	snprintf(usermask, sizeof usermask, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);
	snprintf(oldusermask, sizeof oldusermask, "%s!%s@%s %s", data->oldnick, u->user, u->host, u->gecos);

	MOWGLI_ITER_FOREACH(n, rwatch_list.head)
	{
		rw = n->data;
		if (!rw->re)
			continue;
		if (regex_match(rw->re, usermask))
		{
			// Only process if they did not match before.
			if (regex_match(rw->re, oldusermask))
				continue;
			if (rw->actions & RWACT_SNOOP)
			{
				slog(LG_INFO, "RWATCH:NICKCHANGE:%s \2%s\2 -> \2%s\2 matches \2%s\2 (reason: \2%s\2)",
						rw->actions & RWACT_KLINE ? "KLINE:" : "",
						data->oldnick, usermask, rw->regex, rw->reason);
			}
			if (rw->actions & RWACT_KLINE)
			{
				if (is_autokline_exempt(u))
					slog(LG_INFO, "rwatch_nickchange(): not klining *@%s (user %s -> %s!%s@%s is autokline exempt but matches %s %s)",
							u->host, data->oldnick, u->nick, u->user, u->host,
							rw->regex, rw->reason);
				else
				{
					slog(LG_VERBOSE, "rwatch_nickchange(): klining *@%s (user %s -> %s!%s@%s matches %s %s)",
							u->host, data->oldnick, u->nick, u->user, u->host,
							rw->regex, rw->reason);
					if (! (u->flags & UF_KLINESENT)) {
						kline_sts("*", "*", u->host, SECONDS_PER_DAY, rw->reason);
						u->flags |= UF_KLINESENT;
					}
				}
			}
			else if (rw->actions & RWACT_QUARANTINE)
			{
				if (is_autokline_exempt(u))
					slog(LG_INFO, "rwatch_newuser(): not qurantining *@%s (user %s!%s@%s is autokline exempt but matches %s %s)",
							u->host, u->nick, u->user, u->host,
							rw->regex, rw->reason);
				else
				{
					slog(LG_VERBOSE, "rwatch_newuser(): quaranting *@%s (user %s!%s@%s matches %s %s)",
							u->host, u->nick, u->user, u->host,
							rw->regex, rw->reason);
					quarantine_sts(service_find("operserv")->me, u, SECONDS_PER_DAY, rw->reason);
				}
			}
		}
	}
}

static struct command os_rwatch = {
	.name           = "RWATCH",
	.desc           = N_("Performs actions on connecting clients matching regexes."),
	.access         = PRIV_USER_AUSPEX,
	.maxparc        = 2,
	.cmd            = &os_cmd_rwatch,
	.help           = { .path = "oservice/rwatch" },
};

static struct command os_rwatch_add = {
	.name           = "ADD",
	.desc           = N_("Adds an entry to the regex watch list."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_rwatch_add,
	.help           = { .path = "" },
};

static struct command os_rwatch_del = {
	.name           = "DEL",
	.desc           = N_("Removes an entry from the regex watch list."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_rwatch_del,
	.help           = { .path = "" },
};

static struct command os_rwatch_list = {
	.name           = "LIST",
	.desc           = N_("Displays the regex watch list."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_rwatch_list,
	.help           = { .path = "" },
};

static struct command os_rwatch_set = {
	.name           = "SET",
	.desc           = N_("Changes actions on an entry in the regex watch list"),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_rwatch_set,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	if (! (os_rwatch_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&os_rwatch_add, os_rwatch_cmds);
	(void) command_add(&os_rwatch_del, os_rwatch_cmds);
	(void) command_add(&os_rwatch_list, os_rwatch_cmds);
	(void) command_add(&os_rwatch_set, os_rwatch_cmds);

	(void) service_named_bind_command("operserv", &os_rwatch);

	(void) hook_add_user_add(&rwatch_newuser);
	(void) hook_add_user_nickchange(&rwatch_nickchange);
	(void) hook_add_db_write(&write_rwatchdb);

	char path[BUFSIZE];
	snprintf(path, BUFSIZE, "%s/%s", datadir, "rwatch.db");
	f = fopen(path, "r");

	if (f)
	{
		load_rwatchdb(path); // because i'm lazy, let's pass path to the function
		fclose(f);
	}
	else
	{
		db_register_type_handler("RW", db_h_rw);
		db_register_type_handler("RR", db_h_rr);
	}

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("operserv/rwatch", MODULE_UNLOAD_CAPABILITY_NEVER)
