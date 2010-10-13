/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENCE.
 *
 * This file contains functionality implementing OperServ RWATCH.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/rwatch", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void rwatch_newuser(hook_user_nick_t *data);
static void rwatch_nickchange(hook_user_nick_t *data);

static void os_cmd_rwatch(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_rwatch_list(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_rwatch_add(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_rwatch_del(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_rwatch_set(sourceinfo_t *si, int parc, char *parv[]);

static void write_rwatchdb(void);
static void load_rwatchdb(void);

mowgli_patricia_t *os_rwatch_cmds;

mowgli_list_t rwatch_list;

#define RWACT_SNOOP 1
#define RWACT_KLINE 2

typedef struct rwatch_ rwatch_t;
struct rwatch_
{
	char *regex;
	int reflags; /* AREGEX_* */
	char *reason;
	int actions; /* RWACT_* */
	atheme_regex_t *re;
};

command_t os_rwatch = { "RWATCH", N_("Performs actions on connecting clients matching regexes."), PRIV_USER_AUSPEX, 2, os_cmd_rwatch, { .path = "oservice/rwatch" } };

command_t os_rwatch_add = { "ADD", N_("Adds an entry to the regex watch list."), AC_NONE, 1, os_cmd_rwatch_add, { .path = "" } };
command_t os_rwatch_del = { "DEL", N_("Removes an entry from the regex watch list."), AC_NONE, 1, os_cmd_rwatch_del, { .path = "" } };
command_t os_rwatch_list = { "LIST", N_("Displays the regex watch list."), AC_NONE, 1, os_cmd_rwatch_list, { .path = "" } };
command_t os_rwatch_set = { "SET", N_("Changes actions on an entry in the regex watch list"), AC_NONE, 1, os_cmd_rwatch_set, { .path = "" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_rwatch);

	os_rwatch_cmds = mowgli_patricia_create(strcasecanon);

	command_add(&os_rwatch_add, os_rwatch_cmds);
	command_add(&os_rwatch_del, os_rwatch_cmds);
	command_add(&os_rwatch_list, os_rwatch_cmds);
	command_add(&os_rwatch_set, os_rwatch_cmds);

	hook_add_event("user_add");
	hook_add_user_add(rwatch_newuser);
	hook_add_event("user_nickchange");
	hook_add_user_nickchange(rwatch_nickchange);

	load_rwatchdb();
}

void _moddeinit(void)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, rwatch_list.head)
	{
		rwatch_t *rw = n->data;

		free(rw->regex);
		free(rw->reason);
		if (rw->re != NULL)
			regex_destroy(rw->re);
		free(rw);

		mowgli_node_delete(n, &rwatch_list);
		mowgli_node_free(n);
	}

	service_named_unbind_command("operserv", &os_rwatch);

	command_delete(&os_rwatch_add, os_rwatch_cmds);
	command_delete(&os_rwatch_del, os_rwatch_cmds);
	command_delete(&os_rwatch_list, os_rwatch_cmds);
	command_delete(&os_rwatch_set, os_rwatch_cmds);

	hook_del_user_add(rwatch_newuser);
	hook_del_user_nickchange(rwatch_nickchange);

	mowgli_patricia_destroy(os_rwatch_cmds, NULL, NULL);
}

static void write_rwatchdb(void)
{
	FILE *f;
	mowgli_node_t *n;
	rwatch_t *rw;

	if (!(f = fopen(DATADIR "/rwatch.db.new", "w")))
	{
		slog(LG_ERROR, "write_rwatchdb(): cannot write rwatch database: %s", strerror(errno));
		return;
	}

	MOWGLI_ITER_FOREACH(n, rwatch_list.head)
	{
		rw = n->data;
		fprintf(f, "RW %d %s\n", rw->reflags, rw->regex);
		fprintf(f, "RR %d %s\n", rw->actions, rw->reason);
	}

	fclose(f);

	if ((rename(DATADIR "/rwatch.db.new", DATADIR "/rwatch.db")) < 0)
	{
		slog(LG_ERROR, "write_rwatchdb(): couldn't rename rwatch database.");
		return;
	}
}

static void load_rwatchdb(void)
{
	FILE *f;
	char *item, rBuf[BUFSIZE * 2];
	rwatch_t *rw = NULL;

	if (!(f = fopen(DATADIR "/rwatch.db", "r")))
	{
		slog(LG_DEBUG, "load_rwatchdb(): cannot open rwatch database: %s", strerror(errno));
		return;
	}

	while (fgets(rBuf, BUFSIZE * 2, f))
	{
		item = strtok(rBuf, " ");
		strip(item);

		if (!strcmp(item, "RW"))
		{
			char *reflagsstr = strtok(NULL, " ");
			char *regex = strtok(NULL, "\n");

			if (!reflagsstr || !regex || rw)
				; /* erroneous, don't add */
			else
			{
				rw = (rwatch_t *)smalloc(sizeof(rwatch_t));

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
				; /* erroneous, don't add */
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
}

static void os_cmd_rwatch(sourceinfo_t *si, int parc, char *parv[])
{
	/* Grab args */
	char *cmd = parv[0];
	command_t *c;

	/* Bad/missing arg */
	if (!cmd)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH ADD|DEL|LIST|SET"));
		return;
	}

	c = command_find(os_rwatch_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		return;
	}

	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void os_cmd_rwatch_add(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	char *pattern;
	char *reason;
	atheme_regex_t *regex;
	rwatch_t *rw;
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
		rwatch_t *t = n->data;

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

	rw = malloc(sizeof(rwatch_t));
	rw->regex = sstrdup(pattern);
	rw->reflags = flags;
	rw->reason = sstrdup(reason);
	rw->actions = RWACT_SNOOP;
	rw->re = regex;

	mowgli_node_add(rw, mowgli_node_create(), &rwatch_list);
	command_success_nodata(si, _("Added \2%s\2 to regex watch list."), pattern);
	logcommand(si, CMDLOG_ADMIN, "RWATCH:ADD: \2%s\2 (reason: \2%s\2)", pattern, reason);
	write_rwatchdb();
}

static void os_cmd_rwatch_del(sourceinfo_t *si, int parc, char *parv[])
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
		rwatch_t *rw = n->data;

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
			free(rw->regex);
			free(rw->reason);
			if (rw->re != NULL)
				regex_destroy(rw->re);
			free(rw);
			mowgli_node_delete(n, &rwatch_list);
			mowgli_node_free(n);
			command_success_nodata(si, _("Removed \2%s\2 from regex watch list."), pattern);
			logcommand(si, CMDLOG_ADMIN, "RWATCH:DEL: \2%s\2", pattern);
			write_rwatchdb();
			return;
		}
	}

	command_fail(si, fault_nochange, _("\2%s\2 not found in regex watch list."), pattern);
}

static void os_cmd_rwatch_list(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, rwatch_list.head)
	{
		rwatch_t *rw = n->data;

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

static void os_cmd_rwatch_set(sourceinfo_t *si, int parc, char *parv[])
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
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH SET /<regex>/[i] [KLINE] [NOKLINE] [SNOOP] [NOSNOOP]"));
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RWATCH SET");
		command_fail(si, fault_badparams, _("Syntax: RWATCH SET /<regex>/[i] [KLINE] [NOKLINE] [SNOOP] [NOSNOOP]"));
		return;
	}
	while (*args == ' ')
		args++;

	if (*args == '\0')
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH SET");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH SET /<regex>/[i] [KLINE] [NOKLINE] [SNOOP] [NOSNOOP]"));
		return;
	}

	opts = args;
	while (*args != '\0')
	{
		if (!strncasecmp(args, "KLINE", 5))
			addflags |= RWACT_KLINE, removeflags &= ~RWACT_KLINE, args += 5;
		else if (!strncasecmp(args, "NOKLINE", 7))
			removeflags |= RWACT_KLINE, addflags &= ~RWACT_KLINE, args += 7;
		else if (!strncasecmp(args, "SNOOP", 5))
			addflags |= RWACT_SNOOP, removeflags &= ~RWACT_SNOOP, args += 5;
		else if (!strncasecmp(args, "NOSNOOP", 7))
			removeflags |= RWACT_SNOOP, addflags &= ~RWACT_SNOOP, args += 7;

		if (*args != '\0' && *args != ' ')
		{
			command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RWATCH SET");
			command_fail(si, fault_badparams, _("Syntax: RWATCH SET /<regex>/[i] [KLINE] [NOKLINE] [SNOOP] [NOSNOOP]"));
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

	MOWGLI_ITER_FOREACH_SAFE(n, tn, rwatch_list.head)
	{
		rwatch_t *rw = n->data;

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
			logcommand(si, CMDLOG_ADMIN, "RWATCH:SET: \2%s\2 \2%s\2", pattern, opts);
			write_rwatchdb();
			return;
		}
	}

	command_fail(si, fault_nosuch_target, _("\2%s\2 not found in regex watch list."), pattern);
}

static void rwatch_newuser(hook_user_nick_t *data)
{
	user_t *u = data->u;
	char usermask[NICKLEN+USERLEN+HOSTLEN+GECOSLEN];
	mowgli_node_t *n;
	rwatch_t *rw;

	/* If the user has been killed, don't do anything. */
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
					kline_sts("*", "*", u->host, 86400, rw->reason);
				}
			}
		}
	}
}

static void rwatch_nickchange(hook_user_nick_t *data)
{
	user_t *u = data->u;
	char usermask[NICKLEN+USERLEN+HOSTLEN+GECOSLEN];
	char oldusermask[NICKLEN+USERLEN+HOSTLEN+GECOSLEN];
	mowgli_node_t *n;
	rwatch_t *rw;

	/* If the user has been killed, don't do anything. */
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
			/* Only process if they did not match before. */
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
					kline_sts("*", "*", u->host, 86400, rw->reason);
				}
			}
		}
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
