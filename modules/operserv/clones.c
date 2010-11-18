/*
 * Copyright (c) 2006-2007 Atheme Development Group
 * Rights to this code are documented in doc/LICENCE.
 *
 * This file contains functionality implementing clone detection.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/clones", true, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

#define DEFAULT_WARN_CLONES	3 /* IPs with more than this are warned about */
#define EXEMPT_GRACE		10 /* exempt IPs exceeding their allowance by this are banned */
#define CLONESDB_VERSION	2

static void clones_newuser(hook_user_nick_t *data);
static void clones_userquit(user_t *u);
static void clones_configready(void *unused);

static void os_cmd_clones(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_clones_kline(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_clones_list(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_clones_addexempt(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_clones_delexempt(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_clones_setexempt(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_clones_listexempt(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_clones_duration(sourceinfo_t *si, int parc, char *parv[]);

static void write_exemptdb(database_handle_t *db);


static void clones_kill_users(void *dummy);

static void db_h_ck(database_handle_t *db, const char *type);
static void db_h_cd(database_handle_t *db, const char *type);
static void db_h_ex(database_handle_t *db, const char *type);
static void db_h_clonesdbv(database_handle_t *db, const char *type);

mowgli_patricia_t *os_clones_cmds;

service_t *serviceinfo;

static mowgli_list_t clone_exempts;
bool kline_enabled;
mowgli_patricia_t *hostlist;
mowgli_heap_t *hostentry_heap;
static long kline_duration;
static int clones_allowed, clones_warn, clones_kill;
static unsigned int clones_dbversion = 1;

static mowgli_list_t clones_kill_queue;

typedef struct cexcept_ cexcept_t;
struct cexcept_
{
	char *ip;
	int allowed;
	int warn;
	char *reason;
	int kill;
	long expires;
};

typedef struct hostentry_ hostentry_t;
struct hostentry_
{
	char ip[HOSTIPLEN];
	mowgli_list_t clients;
	time_t lastaction;
	unsigned int lastaction_clones;
};

static inline bool cexempt_expired(cexcept_t *c)
{
	if (c && c->expires && CURRTIME > c->expires)
		return true;

	return false;
}

command_t os_clones = { "CLONES", N_("Manages network wide clones."), PRIV_AKILL, 5, os_cmd_clones, { .path = "oservice/clones" } };

command_t os_clones_kline = { "KLINE", N_("Enables/disables klines for excessive clones."), AC_NONE, 1, os_cmd_clones_kline, { .path = "" } };
command_t os_clones_list = { "LIST", N_("Lists clones on the network."), AC_NONE, 0, os_cmd_clones_list, { .path = "" } };
command_t os_clones_addexempt = { "ADDEXEMPT", N_("Adds a clones exemption."), AC_NONE, 3, os_cmd_clones_addexempt, { .path = "" } };
command_t os_clones_delexempt = { "DELEXEMPT", N_("Deletes a clones exemption."), AC_NONE, 1, os_cmd_clones_delexempt, { .path = "" } };
command_t os_clones_setexempt = { "SETEXEMPT", N_("Sets a clone exemption details."), AC_NONE, 1, os_cmd_clones_setexempt, { .path = "" } };
command_t os_clones_listexempt = { "LISTEXEMPT", N_("Lists clones exemptions."), AC_NONE, 0, os_cmd_clones_listexempt, { .path = "" } };
command_t os_clones_duration = { "DURATION", N_("Sets a custom duration to ban clones for."), AC_NONE, 1, os_cmd_clones_duration, { .path = "" } };

static void clones_configready(void *unused)
{
	clones_allowed = config_options.default_clone_allowed;
	clones_warn = config_options.default_clone_warn;
	clones_kill = config_options.default_clone_limit;
}

static void clones_kill_users(void *dummy)
{
	mowgli_node_t *n, *tn;
	user_t *u;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, clones_kill_queue.head)
	{
		u = n->data;
		kill_user(serviceinfo->me, u, "Too many connections from this host.");

		/* This node delete() and free() will be handled inside the user_quit()
		 * mowgli_node_delete(n, &clones_kill_queue);
		 * mowgli_node_free(n);
		 */
	}
}

void _modinit(module_t *m)
{
	user_t *u;
	mowgli_patricia_iteration_state_t state;

	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->header->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	service_named_bind_command("operserv", &os_clones);

	os_clones_cmds = mowgli_patricia_create(strcasecanon);

	command_add(&os_clones_kline, os_clones_cmds);
	command_add(&os_clones_list, os_clones_cmds);
	command_add(&os_clones_addexempt, os_clones_cmds);
	command_add(&os_clones_delexempt, os_clones_cmds);
	command_add(&os_clones_setexempt, os_clones_cmds);
	command_add(&os_clones_listexempt, os_clones_cmds);
	command_add(&os_clones_duration, os_clones_cmds);

	hook_add_event("config_ready");
	hook_add_config_ready(clones_configready);

	hook_add_event("user_add");
	hook_add_user_add(clones_newuser);
	hook_add_event("user_delete");
	hook_add_user_delete(clones_userquit);
	hook_add_db_write(write_exemptdb);

	db_register_type_handler("CLONES-DBV", db_h_clonesdbv);
	db_register_type_handler("CLONES-CK", db_h_ck);
	db_register_type_handler("CLONES-CD", db_h_cd);
	db_register_type_handler("CLONES-EX", db_h_ex);

	hostlist = mowgli_patricia_create(noopcanon);
	hostentry_heap = mowgli_heap_create(sizeof(hostentry_t), HEAP_USER, BH_NOW);

	kline_duration = 3600; /* set a default */

	serviceinfo = service_find("operserv");


	/* add everyone to host hash */
	MOWGLI_PATRICIA_FOREACH(u, &state, userlist)
	{
		clones_newuser(&(hook_user_nick_t){ .u = u });
	}
}

static void free_hostentry(const char *key, void *data, void *privdata)
{
	mowgli_node_t *n, *tn;
	hostentry_t *he = data;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, he->clients.head)
	{
		mowgli_node_delete(n, &he->clients);
		mowgli_node_free(n);
	}

	mowgli_heap_free(hostentry_heap, he);
}

void _moddeinit(void)
{
	mowgli_node_t *n, *tn;

	mowgli_patricia_destroy(hostlist, free_hostentry, NULL);
	mowgli_heap_destroy(hostentry_heap);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, clone_exempts.head)
	{
		cexcept_t *c = n->data;

		free(c->ip);
		free(c->reason);
		free(c);

		mowgli_node_delete(n, &clone_exempts);
		mowgli_node_free(n);
	}

	if (MOWGLI_LIST_LENGTH(&clones_kill_queue) > 0)
	{
		/* Cannot safely delete users from here, so just forget
		 * about them.
		 */
		event_delete(clones_kill_users, NULL);
		MOWGLI_ITER_FOREACH_SAFE(n, tn, clones_kill_queue.head)
		{
			mowgli_node_delete(n, &clones_kill_queue);
			mowgli_node_free(n);
		}
	}

	service_named_unbind_command("operserv", &os_clones);

	command_delete(&os_clones_kline, os_clones_cmds);
	command_delete(&os_clones_list, os_clones_cmds);
	command_delete(&os_clones_addexempt, os_clones_cmds);
	command_delete(&os_clones_delexempt, os_clones_cmds);
	command_delete(&os_clones_setexempt, os_clones_cmds);
	command_delete(&os_clones_listexempt, os_clones_cmds);

	command_delete(&os_clones_duration, os_clones_cmds);

	hook_del_user_add(clones_newuser);
	hook_del_user_delete(clones_userquit);
	hook_del_db_write(write_exemptdb);
	hook_del_config_ready(clones_configready);

	db_unregister_type_handler("CLONES-DBV");
	db_unregister_type_handler("CLONES-CK");
	db_unregister_type_handler("CLONES-CD");
	db_unregister_type_handler("CLONES-EX");

	mowgli_patricia_destroy(os_clones_cmds, NULL, NULL);
}

static void write_exemptdb(database_handle_t *db)
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

	MOWGLI_ITER_FOREACH_SAFE(n, tn, clone_exempts.head)
	{
		cexcept_t *c = n->data;
		if (cexempt_expired(c))
		{
			free(c->ip);
			free(c->reason);
			free(c);
			mowgli_node_delete(n, &clone_exempts);
			mowgli_node_free(n);
		}
		else
		{
			db_start_row(db, "CLONES-EX");
			db_write_word(db, c->ip);
			db_write_uint(db, c->allowed);
			db_write_uint(db, c->warn);
			db_write_uint(db, c->kill);
			db_write_time(db, c->expires);
			db_write_str(db, c->reason);
			db_commit_row(db);
		}
	}
}

static void db_h_clonesdbv(database_handle_t *db, const char *type)
{
	clones_dbversion = db_sread_uint(db);
}
static void db_h_ck(database_handle_t *db, const char *type)
{
	kline_enabled = db_sread_int(db) != 0;
}

static void db_h_cd(database_handle_t *db, const char *type)
{
	kline_duration = db_sread_uint(db);
}

static void db_h_ex(database_handle_t *db, const char *type)
{
	unsigned int allowed, warn, clonekill;

	const char *ip = db_sread_word(db);
	allowed = db_sread_uint(db);

	if (clones_dbversion == 2)
	{
		warn = db_sread_uint(db);
		clonekill = db_sread_uint(db);
	} 
	else
	{
		warn = allowed;
		clonekill = allowed+1;
	}

	time_t expires = db_sread_time(db);
	const char *reason = db_sread_str(db);

	cexcept_t *c = (cexcept_t *)smalloc(sizeof(cexcept_t));
	c->ip = sstrdup(ip);
	c->allowed = allowed;
	c->warn = warn;
	c->kill = clonekill;
	c->expires = expires;
	c->reason = sstrdup(reason);
	mowgli_node_add(c, mowgli_node_create(), &clone_exempts);
}

static unsigned int is_exempt(const char *ip)
{
	mowgli_node_t *n;

	/* first check for an exact match */
	MOWGLI_ITER_FOREACH(n, clone_exempts.head)
	{
		cexcept_t *c = n->data;

		if (!strcmp(ip, c->ip))
			return c->allowed;
	}

	/* then look for cidr */
	MOWGLI_ITER_FOREACH(n, clone_exempts.head)
	{
		cexcept_t *c = n->data;

		if (!match_ips(c->ip, ip))
			return c->allowed;
	}

	return 0;
}

static cexcept_t * find_exempt(const char *ip)
{
	mowgli_node_t *n;

	/* first check for an exact match */
	MOWGLI_ITER_FOREACH(n, clone_exempts.head)
	{
		cexcept_t *c = n->data;

		if (!strcmp(ip, c->ip))
			return c;
	}

	/* then look for cidr */
	MOWGLI_ITER_FOREACH(n, clone_exempts.head)
	{
		cexcept_t *c = n->data;

		if (!match_ips(c->ip, ip))
			return c;
	}

	return 0;
}

static void os_cmd_clones(sourceinfo_t *si, int parc, char *parv[])
{
	command_t *c;
	char *cmd = parv[0];
	
	/* Bad/missing arg */
	if (!cmd)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES");
		command_fail(si, fault_needmoreparams, _("Syntax: CLONES KLINE|LIST|ADDEXEMPT|DELEXEMPT|LISTEXEMPT|SETEXEMPT [parameters]"));
		return;
	}
	
	c = command_find(os_clones_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		return;
	}

	command_exec(si->service, si, c, parc + 1, parv + 1);
}

static void os_cmd_clones_kline(sourceinfo_t *si, int parc, char *parv[])
{
	const char *arg = parv[0];

	if (arg == NULL)
		arg = "";

	if (!strcasecmp(arg, "ON"))
	{
		if (kline_enabled)
		{
			command_fail(si, fault_nochange, _("CLONES klines are already enabled."));
			return;
		}
		kline_enabled = true;
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
	else
	{
		if (kline_enabled)
			command_success_string(si, "ON", _("CLONES klines are currently enabled."));
		else
			command_success_string(si, "OFF", _("CLONES klines are currently disabled."));
	}
}

static void os_cmd_clones_list(sourceinfo_t *si, int parc, char *parv[])
{
	hostentry_t *he;
	int k = 0;
	mowgli_patricia_iteration_state_t state;
	int allowed = 0;

	MOWGLI_PATRICIA_FOREACH(he, &state, hostlist)
	{
		k = MOWGLI_LIST_LENGTH(&he->clients);

		if (k > 3)
		{
			if ((allowed = is_exempt(he->ip)))
				command_success_nodata(si, _("%d from %s (\2EXEMPT\2; allowed %d)"), k, he->ip, allowed);
			else
				command_success_nodata(si, _("%d from %s"), k, he->ip);
		}
	}
	command_success_nodata(si, _("End of CLONES LIST"));
	logcommand(si, CMDLOG_ADMIN, "CLONES:LIST");
}

static void os_cmd_clones_addexempt(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	char *ip = parv[0];
	char *clonesstr = parv[1];
	int clones;
	char *expiry = parv[2];
	char *reason = parv[3];
	char rreason[BUFSIZE];
	cexcept_t *c = NULL;
	long duration;

	if (!ip || !clonesstr || !expiry)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES ADDEXEMPT");
		command_fail(si, fault_needmoreparams, _("Syntax: CLONES ADDEXEMPT <ip> <clones> [!P|!T <minutes>] <reason>"));
		return;
	}

	clones = atoi(clonesstr);
	if (clones <= DEFAULT_WARN_CLONES)
	{
		command_fail(si, fault_badparams, _("Allowed clones count must be more than %d"), DEFAULT_WARN_CLONES);
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
		strlcpy(rreason, reason, BUFSIZE);
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

		if (expiry)
		{
			duration = (atol(expiry) * 60);
			while (isdigit(*expiry))
				++expiry;
			if (*expiry == 'h' || *expiry == 'H')
				duration *= 60;
			else if (*expiry == 'd' || *expiry == 'D')
				duration *= 1440;
			else if (*expiry == 'w' || *expiry == 'W')
				duration *= 10080;
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
		}
		else
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES ADDEXEMPT");
			command_fail(si, fault_needmoreparams, _("Syntax: AKILL ADD <nick|hostmask> [!P|!T <minutes>] <reason>"));
			return;
		}

		if (!reason)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES ADDEXEMPT");
			command_fail(si, fault_needmoreparams, _("Syntax: CLONES ADDEXEMPT <ip> <clones> [!P|!T <minutes>] <reason>"));
			return;
		}

		strlcpy(rreason, reason, BUFSIZE);
	}
	else if (expiry)
	{
		duration = config_options.clone_time;

		strlcpy(rreason, expiry, BUFSIZE);
		if (reason)
		{
			strlcat(rreason, " ", BUFSIZE);
			strlcat(rreason, reason, BUFSIZE);
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
		cexcept_t *t = n->data;

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

		c = smalloc(sizeof(cexcept_t));
		c->ip = sstrdup(ip);
		c->reason = sstrdup(rreason);
		mowgli_node_add(c, mowgli_node_create(), &clone_exempts);
		command_success_nodata(si, _("Added \2%s\2 to clone exempt list."), ip);
	}
	else
	{
		if (*rreason)
		{
			free(c->reason);
			c->reason = sstrdup(rreason);
		}
		command_success_nodata(si, _("\2Warning\2: the syntax you are using to update this exemption has been deprecated and may be removed in a future version.  Please use SETEXEMPT in the future instead."));
		command_success_nodata(si, _("Updated \2%s\2 in clone exempt list."), ip);
	}

	c->allowed = clones;
	c->warn = clones;
	c->kill = clones+1;
	c->expires = duration ? (CURRTIME + duration) : 0;

	logcommand(si, CMDLOG_ADMIN, "CLONES:ADDEXEMPT: \2%s\2 \2%d\2 (reason: \2%s\2) (duration: \2%s\2)", ip, clones, c->reason, timediff(duration));
}

static void os_cmd_clones_delexempt(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n, *tn;
	char *arg = parv[0];

	if (!arg)
		return;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, clone_exempts.head)
	{
		cexcept_t *c = n->data;

		if (cexempt_expired(c))
		{
			free(c->ip);
			free(c->reason);
			free(c);
			mowgli_node_delete(n, &clone_exempts);
			mowgli_node_free(n);
		}
		else if (!strcmp(c->ip, arg))
		{
			free(c->ip);
			free(c->reason);
			free(c);
			mowgli_node_delete(n, &clone_exempts);
			mowgli_node_free(n);
			command_success_nodata(si, _("Removed \2%s\2 from clone exempt list."), arg);
			logcommand(si, CMDLOG_ADMIN, "CLONES:DELEXEMPT: \2%s\2", arg);
			return;
		}
	}

	command_fail(si, fault_nosuch_target, _("\2%s\2 not found in clone exempt list."), arg);
}


static void os_cmd_clones_setexempt(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n, *tn;
	char *ip = parv[0];
	char *subcmd = parv[1];
	char *clonesstr = parv[2];
	char *reason = parv[3];
	int clones;
	char rreason[BUFSIZE];

	long duration;

	if (!ip || !subcmd || !clonesstr)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONES SETEXEMPT");
		command_fail(si, fault_needmoreparams, _("Syntax: CLONES SETEXEMPT [DEFAULT | <ip>] <ALLOWED | WARN | KILL> <limit>"));
		command_fail(si, fault_needmoreparams, _("Syntax: CLONES SETEXEMPT <ip> <REASON | DURATION> <value>"));
		return;
	}

	if (!strcasecmp(ip,"DEFAULT"))
	{
		clones = atoi(clonesstr);
		if (!strcasecmp(subcmd,"ALLOWED"))
		{
			if (clones < DEFAULT_WARN_CLONES)
			{
				command_fail(si, fault_badparams, _("Default allowed clone limit must be more than %d."),DEFAULT_WARN_CLONES);
				return;
			}
			else if (clones > clones_warn)
			{
				command_fail(si, fault_badparams, _("Default allowed clone limit cannot be higher than the default warn limit (currently %d)."),clones_warn);
				return;
			}
			else if (clones >= clones_kill)
			{
				command_fail(si, fault_badparams, _("Default allowed clone limit must be lower than the default kill limit (currently %d)."),clones_kill);
				return;
			}

			clones_allowed = clones;
			command_success_nodata(si,_("Default allowed clone limit set to \2%d\2."),clones_allowed);
		}
		else if (!strcasecmp(subcmd,"WARN"))
		{
			if (clones == 0)
			{
				command_success_nodata(si,_("Default clone warning has been disabled."));
				clones_warn = 0;
				return;
			}
			else if (clones < clones_allowed)
			{
				command_fail(si, fault_badparams, _("Default warned clone limit cannot be lower than the default allowed limit (currently %d)."), clones_allowed);
				return;
			}
			else if (clones >= clones_kill && clones_kill > 0)
			{
				command_fail(si, fault_badparams, _("Default warned clone limit must be less than the default kill limit (currently %d)."), clones_kill);
				return;
			}

			clones_warn = clones;
			command_success_nodata(si,_("Default warned clone limit set to \2%d\2"),clones_warn);
		}
		else if (!strcasecmp(subcmd,"KILL"))
		{
			if (clones == 0)
			{
				command_success_nodata(si,_("Default clone kill limit has been disabled."));
			}
			else if (clones <= clones_warn && clones_warn > 0)
			{
				command_fail(si, fault_badparams, _("Default kill clone limit must be more than the default warn limit (currently %d)."), clones_warn);
				return;
			}
			else if (clones <= clones_allowed)
			{
				command_fail(si, fault_badparams, _("Default kill clone limit must be more than the default allowed limit (currently %d) since warnings are disabled."), clones_allowed);
				return;
			}

			clones_kill = clones;
			command_success_nodata(si,_("Default kill clone limit set to \2%d\2"),clones_kill);
		}
		else
		{
			/* Invalid parameters */
			command_fail(si, fault_badparams, _("Invalid syntax given."));
			command_fail(si, fault_badparams, _("Syntax: CLONES SETEXEMPT DEFAULT <ALLOWED | WARN | KILL> <limit>"));
			return;
		}
	}
	else if (ip) {
		MOWGLI_ITER_FOREACH_SAFE(n, tn, clone_exempts.head)
		{
			cexcept_t *c = n->data;

			if (cexempt_expired(c))
			{
				free(c->ip);
				free(c->reason);
				free(c);
				mowgli_node_delete(n, &clone_exempts);
				mowgli_node_free(n);
			}
			else if (!strcmp(c->ip, ip))
			{
				if (!strcasecmp(subcmd,"ALLOWED"))
				{
					clones = atoi(clonesstr);
					if (clones < DEFAULT_WARN_CLONES)
					{
						command_fail(si, fault_badparams, _("Allowed clones must be more than %d."),DEFAULT_WARN_CLONES);
						return;
					}
					else if (clones > c->warn)
					{
						command_fail(si, fault_badparams, _("The ALLOWED clone limit must be lower than the WARN limit (currently %d)"),c->warn);
						return;
					}
					else if (clones >= c->kill)
					{
						command_fail(si, fault_badparams, _("The ALLOWED clone limit must be lower than the KILL limit (currently %d)"),c->kill);
						return;
					}

					c->allowed = clones;
					command_success_nodata(si,_("Allowed clones limit for host \2%s\2 set to \2%d\2"),ip,c->allowed);
				}
				else if (!strcasecmp(subcmd,"WARN"))
				{
					clones = atoi(clonesstr);
					if (clones == 0)
					{
						command_success_nodata(si,_("Clone warning messages will be disabled for host \2%s\2"),ip);
						c->warn = 0;
						return;
					}
					else if (clones < c->allowed)
					{
						command_fail(si, fault_badparams, _("Warned clones limit cannot be less than the allowed limit of %d"), c->allowed);
						return;
					}
					else if (clones >= c->kill && c->kill > 0)
					{
						command_fail(si, fault_badparams, _("Warned clones limit must be less than the kill limit of %d."), c->kill);
						return;
					}

					c->warn = clones;
					command_success_nodata(si,_("Warned clones limit for host \2%s\2 set to \2%d\2"),ip,c->warn);
				}
				else if (!strcasecmp(subcmd,"KILL"))
				{
					clones = atoi(clonesstr);
					if (clones == 0)
					{
						command_success_nodata(si,_("Clone kills will be disabled for host \2%s\2"),ip);
						c->kill = 0;
						return;
					}
					else if (clones <= clones_warn)
					{
						command_fail(si, fault_badparams, _("Killed clones limit must be more than the warn limit of %d"), c->warn);
						return;
					}

					c->kill = clones;
					command_success_nodata(si,_("Kill clone limit for host \2%s\2 set to \2%d\2"),ip,c->kill);
				}
				else if (!strcasecmp(subcmd,"DURATION"))
				{
					char *expiry = clonesstr;
					if (!strcmp(expiry,"0"))
					{
						c->expires = 0;
						command_success_nodata(si,_("Clone exemption duration for host \2%s\2 set to \2permanent\2"),ip);
					}
					else
					{
						duration = (atol(expiry) * 60);
						while (isdigit(*expiry))
							++expiry;
						if (*expiry == 'h' || *expiry == 'H')
							duration *= 60;
						else if (*expiry == 'd' || *expiry == 'D')
							duration *= 1440;
						else if (*expiry == 'w' || *expiry == 'W')
							duration *= 10080;
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
						command_success_nodata(si,_("Clone exemption duration for host \2%s\2 set to \2%s\2 (%ld seconds)"),ip,clonesstr,duration);
					}

				}
				else if (!strcasecmp(subcmd,"REASON"))
				{
					strlcpy(rreason, clonesstr, BUFSIZE);
					if (reason)
					{
						strlcat(rreason, " ", BUFSIZE);
						strlcat(rreason, reason, BUFSIZE);
					}
					/* strlcat(rreason,clonesstr,BUFSIZE); */
					free(c->reason);
					c->reason = sstrdup(rreason);
					command_success_nodata(si,_("Clone exemption reason for host \2%s\2 changed to \2%s\2"),ip,c->reason);
				}
				else
				{
					/* Invalid parameters */
					command_fail(si, fault_badparams, _("Invalid syntax given."));
					command_fail(si, fault_badparams, _("Syntax: CLONES SETEXEMPT <IP> <ALLOWED | WARN | KILL | DURATION | REASON> <value>"));
					return;
				}
				return;
			}
		}

		command_fail(si, fault_nosuch_target, _("\2%s\2 not found in clone exempt list."), ip);
	}
}

static void os_cmd_clones_duration(sourceinfo_t *si, int parc, char *parv[])
{
	char *s = parv[0];
	long duration;

	if (!s)
	{
		command_success_nodata(si, _("Clone ban duration set to \2%ld\2 (%ld seconds)"), kline_duration / 60, kline_duration);
		return;
	}

	duration = (atol(s) * 60);
	while (isdigit(*s))
		s++;
	if (*s == 'h' || *s == 'H')
		duration *= 60;
	else if (*s == 'd' || *s == 'D')
		duration *= 1440;
	else if (*s == 'w' || *s == 'W')
		duration *= 10080;
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

static void os_cmd_clones_listexempt(sourceinfo_t *si, int parc, char *parv[])
{

	command_success_nodata(si, "DEFAULT - allowed limit %d, warn on %d, kill after %d",clones_allowed,clones_warn,clones_kill);
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, clone_exempts.head)
	{
		cexcept_t *c = n->data;

		if (cexempt_expired(c))
		{
			free(c->ip);
			free(c->reason);
			free(c);
			mowgli_node_delete(n, &clone_exempts);
			mowgli_node_free(n);
		}
		else if (c->expires)
			command_success_nodata(si, "%s - allowed limit %d, warn on %d, kill after %d - expires in %s - \2%s\2", c->ip, c->allowed, c->warn, c->kill, timediff(c->expires > CURRTIME ? c->expires - CURRTIME : 0), c->reason);
		else
			command_success_nodata(si, "%s - allowed limit %d, warn on %d, kill after %d - \2permanent\2 - \2%s\2", c->ip, c->allowed, c->warn, c->kill, c->reason);
	}
	command_success_nodata(si, _("End of CLONES LISTEXEMPT"));
	logcommand(si, CMDLOG_ADMIN, "CLONES:LISTEXEMPT");
}

static void clones_newuser(hook_user_nick_t *data)
{
	user_t *u = data->u;
	unsigned int i;
	hostentry_t *he;
	unsigned int allowed, warn, clonekill;

	/* If the user has been killed, don't do anything. */
	if (!u)
		return;

	/* User has no IP, ignore him */
	if (is_internal_client(u) || *u->ip == '\0')
		return;

	he = mowgli_patricia_retrieve(hostlist, u->ip);
	if (he == NULL)
	{
		he = mowgli_heap_alloc(hostentry_heap);
		strlcpy(he->ip, u->ip, sizeof he->ip);
		mowgli_patricia_add(hostlist, he->ip, he);
	}
	mowgli_node_add(u, mowgli_node_create(), &he->clients);
	i = MOWGLI_LIST_LENGTH(&he->clients);

	cexcept_t *c = find_exempt(u->ip);
	if (c == 0)
	{
		allowed = clones_allowed;
		warn = clones_warn;
		clonekill = clones_kill;
	}
	else
	{
		allowed = clones_allowed > c->allowed ? clones_allowed : c->allowed;
		warn = clones_warn > c->warn && c->warn > 0 ? clones_warn : c->warn;
		clonekill = clones_kill > c->kill && c->kill > 0? clones_kill : c->kill;
	}

	if (i >= allowed)
	{
		if (i >= allowed && (i < clonekill || clonekill == 0))
		{
			slog(LG_INFO, "CLONES: \2%d\2 clones on \2%s\2 (%s!%s@%s) (\2%d\2 allowed)", i, u->ip, u->nick, u->user, u->host, allowed);
			if (i >= warn && warn > 0)
				msg(serviceinfo->nick, u->nick, "\2WARNING\2: You may not have any more than \2%d\2 clients on the network at once. Any further connections over this limit risks being removed.", allowed);
		}
		else if (i >= clonekill && is_autokline_exempt(u) && clonekill > 0)
			slog(LG_INFO, "CLONES: \2%d\2 clones on \2%s\2 (%s!%s@%s) (user is autokline exempt)", i, u->ip, u->nick, u->user, u->host);
		else if (i >= clonekill && !kline_enabled && clonekill > 0)
		{
			slog(LG_INFO, "CLONES: \2%d\2 clones on \2%s\2 (%s!%s@%s) (TKLINE disabled, killing user)", i, u->ip, u->nick, u->user, u->host);

			if (MOWGLI_LIST_LENGTH(&clones_kill_queue) == 0)
			{
				event_add_once("clones_kill_users", clones_kill_users, NULL, 0);
			}

			if (!mowgli_node_find(u, &clones_kill_queue))
			{
				mowgli_node_add(u, mowgli_node_create(), &clones_kill_queue);
			}
		}
		else if (i >= clonekill && clonekill > 0)
		{
			slog(LG_INFO, "CLONES: \2%d\2 clones on \2%s\2 (%s!%s@%s) (TKLINE due to excess clones)", i, u->ip, u->nick, u->user, u->host);
			kline_sts("*", "*", u->ip, kline_duration, "Excessive clones");
		}

	}
}

static void clones_userquit(user_t *u)
{
	mowgli_node_t *n;
	hostentry_t *he;

	/* User has no IP, ignore him */
	if (is_internal_client(u) || *u->ip == '\0')
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
			mowgli_patricia_delete(hostlist, he->ip);
			mowgli_heap_free(hostentry_heap, he);
		}
	}

	n = mowgli_node_find(u, &clones_kill_queue);
	if (n != NULL)
	{
		mowgli_node_delete(n, &clones_kill_queue);
		mowgli_node_free(n);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
