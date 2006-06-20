/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the routines that deal with the configuration.
 *
 * $Id: conf.c 5466 2006-06-20 23:24:29Z jilles $
 */

#include "atheme.h"

#define PARAM_ERROR(ce) { slog(LG_INFO, "%s:%i: no parameter for " \
                          "configuration option: %s", \
                          (ce)->ce_fileptr->cf_filename, \
                          (ce)->ce_varlinenum, (ce)->ce_varname); \
  return 1; }

static int c_serverinfo(CONFIGENTRY *);
static int c_cservice(CONFIGENTRY *);
static int c_gservice(CONFIGENTRY *);
static int c_oservice(CONFIGENTRY *);
static int c_general(CONFIGENTRY *);
static int c_database(CONFIGENTRY *);
static int c_uplink(CONFIGENTRY *);
static int c_nickserv(CONFIGENTRY *);
static int c_userserv(CONFIGENTRY *);
static int c_saslserv(CONFIGENTRY *);
static int c_memoserv(CONFIGENTRY *);
static int c_helpserv(CONFIGENTRY *);
static int c_loadmodule(CONFIGENTRY *);
static int c_operclass(CONFIGENTRY *);
static int c_operator(CONFIGENTRY *);
static int c_language(CONFIGENTRY *);
static int c_string(CONFIGENTRY *);

static int c_si_name(CONFIGENTRY *);
static int c_si_desc(CONFIGENTRY *);
static int c_si_uplink(CONFIGENTRY *);
static int c_si_numeric(CONFIGENTRY *);
static int c_si_vhost(CONFIGENTRY *);
static int c_si_recontime(CONFIGENTRY *);
static int c_si_restarttime(CONFIGENTRY *);
static int c_si_netname(CONFIGENTRY *);
static int c_si_hidehostsuffix(CONFIGENTRY *);
static int c_si_adminname(CONFIGENTRY *);
static int c_si_adminemail(CONFIGENTRY *);
static int c_si_mta(CONFIGENTRY *);
static int c_si_loglevel(CONFIGENTRY *);
static int c_si_maxlogins(CONFIGENTRY *);
static int c_si_maxusers(CONFIGENTRY *);
static int c_si_maxchans(CONFIGENTRY *);
static int c_si_emaillimit(CONFIGENTRY *);
static int c_si_emailtime(CONFIGENTRY *);
static int c_si_auth(CONFIGENTRY *);
static int c_si_mdlimit(CONFIGENTRY *);
static int c_si_casemapping(CONFIGENTRY *);

/* CService client information. */
static int c_ci_nick(CONFIGENTRY *);
static int c_ci_user(CONFIGENTRY *);
static int c_ci_host(CONFIGENTRY *);
static int c_ci_real(CONFIGENTRY *);
static int c_ci_fantasy(CONFIGENTRY *);
static int c_ci_vop(CONFIGENTRY *);
static int c_ci_hop(CONFIGENTRY *);
static int c_ci_aop(CONFIGENTRY *);
static int c_ci_sop(CONFIGENTRY *);
static int c_ci_changets(CONFIGENTRY *);

/* GService client information. */
static int c_gl_nick(CONFIGENTRY *);
static int c_gl_user(CONFIGENTRY *);
static int c_gl_host(CONFIGENTRY *);
static int c_gl_real(CONFIGENTRY *);

/* OService client information. */
static int c_oi_nick(CONFIGENTRY *);
static int c_oi_user(CONFIGENTRY *);
static int c_oi_host(CONFIGENTRY *);
static int c_oi_real(CONFIGENTRY *);

/* NickServ client information. */
static int c_ni_nick(CONFIGENTRY *);
static int c_ni_user(CONFIGENTRY *);
static int c_ni_host(CONFIGENTRY *);
static int c_ni_real(CONFIGENTRY *);
static int c_ni_spam(CONFIGENTRY *);

/* UserServ client information. */
static int c_ui_nick(CONFIGENTRY *);
static int c_ui_user(CONFIGENTRY *);
static int c_ui_host(CONFIGENTRY *);
static int c_ui_real(CONFIGENTRY *);

/* SaslServ client information. */
static int c_ss_nick(CONFIGENTRY *);
static int c_ss_user(CONFIGENTRY *);
static int c_ss_host(CONFIGENTRY *);
static int c_ss_real(CONFIGENTRY *);

/* MemoServ client information. */
static int c_ms_nick(CONFIGENTRY *);
static int c_ms_user(CONFIGENTRY *);
static int c_ms_host(CONFIGENTRY *);
static int c_ms_real(CONFIGENTRY *);

/* HelpServ client information. */
static int c_hs_nick(CONFIGENTRY *);
static int c_hs_user(CONFIGENTRY *);
static int c_hs_host(CONFIGENTRY *);
static int c_hs_real(CONFIGENTRY *);

/* Database information. */
static int c_db_user(CONFIGENTRY *);
static int c_db_host(CONFIGENTRY *);
static int c_db_password(CONFIGENTRY *);
static int c_db_database(CONFIGENTRY *);
static int c_db_port(CONFIGENTRY *);

/* language:: stuff */
static int c_la_name(CONFIGENTRY *);
static int c_la_translator(CONFIGENTRY *);

static int c_gi_chan(CONFIGENTRY *);
static int c_gi_silent(CONFIGENTRY *);
static int c_gi_verbose_wallops(CONFIGENTRY *);
static int c_gi_use_privmsg(CONFIGENTRY *);
static int c_gi_join_chans(CONFIGENTRY *);
static int c_gi_leave_chans(CONFIGENTRY *);
static int c_gi_uflags(CONFIGENTRY *);
static int c_gi_cflags(CONFIGENTRY *);
static int c_gi_raw(CONFIGENTRY *);
static int c_gi_flood_msgs(CONFIGENTRY *);
static int c_gi_flood_time(CONFIGENTRY *);
static int c_gi_kline_time(CONFIGENTRY *);
static int c_gi_commit_interval(CONFIGENTRY *);
static int c_gi_expire(CONFIGENTRY *);
static int c_gi_sras(CONFIGENTRY *);
static int c_gi_secure(CONFIGENTRY *);

static BlockHeap *conftable_heap;

/* *INDENT-OFF* */

static struct Token uflags[] = {
  { "HOLD",     MU_HOLD     },
  { "NEVEROP",  MU_NEVEROP  },
  { "NOOP",     MU_NOOP     },
  { "HIDEMAIL", MU_HIDEMAIL },
  { "NONE",     0           },
  { NULL, 0 }
};

static struct Token cflags[] = {
  { "HOLD",        MC_HOLD        },
  { "SECURE",      MC_SECURE      },
  { "VERBOSE",     MC_VERBOSE     },
  { "KEEPTOPIC",   MC_KEEPTOPIC   },
  { "VERBOSE_OPS", MC_VERBOSE_OPS },
  { "NONE",        0              },
  { NULL, 0 }
};

list_t confblocks;
list_t conf_si_table;
list_t conf_ci_table;
list_t conf_gl_table;
list_t conf_oi_table;
list_t conf_ni_table;
list_t conf_db_table;
list_t conf_gi_table;
list_t conf_ui_table;
list_t conf_ms_table;
list_t conf_hs_table;
list_t conf_la_table;
list_t conf_ss_table;

/* *INDENT-ON* */

static void conf_process(CONFIGFILE *cfp)
{
	CONFIGFILE *cfptr;
	CONFIGENTRY *ce;
	node_t *tn;
	struct ConfTable *ct = NULL;

	for (cfptr = cfp; cfptr; cfptr = cfptr->cf_next)
	{
		for (ce = cfptr->cf_entries; ce; ce = ce->ce_next)
		{
			LIST_FOREACH(tn, confblocks.head)
			{
				ct = tn->data;

				if (!strcasecmp(ct->name, ce->ce_varname))
				{
					ct->handler(ce);
					break;
				}
			}
			if (!ct)
				slog(LG_INFO, "%s:%d: invalid configuration option: %s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_varname);
		}
	}
}

boolean_t conf_parse(char *file)
{
	CONFIGFILE *cfp;

	cfp = config_load(file);
	if (cfp == NULL)
	{
		slog(LG_ERROR, "conf_parse(): unable to load configuration file: %s", strerror(errno));

		return FALSE;
	}

	conf_process(cfp);
	config_free(cfp);

	if (!pmodule_loaded)
	{
		slog(LG_ERROR, "No protocol module loaded, aborting");
		exit(EXIT_FAILURE);
	}

	hook_call_event("config_ready", NULL);
	return TRUE;
}

void conf_init(void)
{
	if (me.netname)
		free(me.netname);
	if (me.hidehostsuffix)
		free(me.hidehostsuffix);
	if (me.adminname)
		free(me.adminname);
	if (me.adminemail)
		free(me.adminemail);
	if (me.mta)
		free(me.mta);
	if (chansvs.nick)
		free(chansvs.nick);
	if (config_options.chan)
		free(config_options.chan);
	if (config_options.global)
		free(config_options.global);
	if (config_options.languagefile)
		free(config_options.languagefile);

	me.netname = me.hidehostsuffix = me.adminname = me.adminemail = me.mta = chansvs.nick = config_options.chan = 
		config_options.global = config_options.languagefile = NULL;

	me.recontime = me.restarttime = me.maxlogins = me.maxusers = me.maxchans = me.emaillimit = me.emailtime = 
		config_options.flood_msgs = config_options.flood_time = config_options.kline_time = config_options.commit_interval =
		config_options.expire = 0;

	/* we don't reset loglevel because too much stuff uses it */
	config_options.defuflags = config_options.defcflags = 0x00000000;

	config_options.silent = config_options.join_chans = config_options.leave_chans = config_options.raw = FALSE;

	me.auth = AUTH_NONE;

	me.mdlimit = 30;

	chansvs.fantasy = FALSE;
	if (chansvs.me != NULL && fcmd_agent == chansvs.me)
		fcmd_agent = NULL;
	chansvs.ca_vop = CA_VOP_DEF;
	chansvs.ca_hop = CA_HOP_DEF;
	chansvs.ca_aop = CA_AOP_DEF;
	chansvs.ca_sop = CA_SOP_DEF;
	chansvs.changets = FALSE;

	if (!(runflags & RF_REHASHING))
	{
		if (me.name)
			free(me.name);
		if (me.desc)
			free(me.desc);
		if (me.uplink)
			free(me.uplink);
		if (me.vhost)
			free(me.vhost);
		if (chansvs.user)
			free(chansvs.user);
		if (chansvs.host)
			free(chansvs.host);
		if (chansvs.real)
			free(chansvs.real);

		me.name = me.desc = me.uplink = me.vhost = chansvs.user = chansvs.host = chansvs.real = NULL;

		set_match_mapping(MATCH_RFC1459);	/* default to RFC compliancy */
	}
}

int subblock_handler(CONFIGENTRY *ce, list_t *entries)
{
	node_t *tn;
	struct ConfTable *ct = NULL;

	for (ce = ce->ce_entries; ce; ce = ce->ce_next)
	{
		LIST_FOREACH(tn, entries->head)
		{
			ct = tn->data;

			if (!strcasecmp(ct->name, ce->ce_varname))
			{
				ct->handler(ce);
				break;
			}
		}
		if (!ct)
			slog(LG_INFO, "%s:%d: invalid configuration option: %s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_varname);
	}
	return 0;
}

struct ConfTable *find_top_conf(char *name)
{
	node_t *n;
	struct ConfTable *ct;

	LIST_FOREACH(n, confblocks.head)
	{
		ct = n->data;

		if (!strcasecmp(ct->name, name))
			return ct;
	}

	return NULL;
}

struct ConfTable *find_conf_item(char *name, list_t *conflist)
{
	node_t *n;
	struct ConfTable *ct;

	LIST_FOREACH(n, conflist->head)
	{
		ct = n->data;

		if (!strcasecmp(ct->name, name))
			return ct;
	}

	return NULL;
}

void add_top_conf(char *name, int (*handler) (CONFIGENTRY *ce))
{
	struct ConfTable *ct;

	if ((ct = find_top_conf(name)))
	{
		slog(LG_DEBUG, "add_top_conf(): duplicate config block '%s'.", name);
		return;
	}

	ct = BlockHeapAlloc(conftable_heap);

	ct->name = sstrdup(name);
	ct->rehashable = 1;
	ct->handler = handler;

	node_add(ct, node_create(), &confblocks);
}

void add_conf_item(char *name, list_t *conflist, int (*handler) (CONFIGENTRY *ce))
{
	struct ConfTable *ct;

	if ((ct = find_conf_item(name, conflist)))
	{
		slog(LG_DEBUG, "add_conf_item(): duplicate item %s", name);
		return;
	}

	ct = BlockHeapAlloc(conftable_heap);

	ct->name = sstrdup(name);
	ct->rehashable = 1;
	ct->handler = handler;

	node_add(ct, node_create(), conflist);
}

void del_top_conf(char *name)
{
	node_t *n;
	struct ConfTable *ct;

	if (!(ct = find_top_conf(name)))
	{
		slog(LG_DEBUG, "del_top_conf(): cannot delete nonexistant block %s", name);
		return;
	}

	n = node_find(ct, &confblocks);
	node_del(n, &confblocks);

	free(ct->name);

	BlockHeapFree(conftable_heap, ct);
}

void del_conf_item(char *name, list_t *conflist)
{
	node_t *n;
	struct ConfTable *ct;

	if (!(ct = find_conf_item(name, conflist)))
	{
		slog(LG_DEBUG, "del_conf_item(): cannot delete nonexistant item %s", name);
		return;
	}

	n = node_find(ct, conflist);
	node_del(n, conflist);

	free(ct->name);

	BlockHeapFree(conftable_heap, ct);
}

void init_newconf(void)
{
	conftable_heap = BlockHeapCreate(sizeof(struct ConfTable), 32);

	if (!conftable_heap)
	{
		slog(LG_INFO, "init_newconf(): block allocator failure.");
		exit(EXIT_FAILURE);
	}

	/* First we set up the blocks. */
	add_top_conf("SERVERINFO", c_serverinfo);
	add_top_conf("CHANSERV", c_cservice);
	add_top_conf("CSERVICE", c_cservice);
	add_top_conf("GLOBAL", c_gservice);
	add_top_conf("GSERVICE", c_gservice);
	add_top_conf("OPERSERV", c_oservice);
	add_top_conf("OSERVICE", c_oservice);
	add_top_conf("NICKSERV", c_nickserv);
	add_top_conf("USERSERV", c_userserv);
	add_top_conf("SASLSERV", c_saslserv);
	add_top_conf("MEMOSERV", c_memoserv);
	add_top_conf("HELPSERV", c_helpserv);
	add_top_conf("UPLINK", c_uplink);
	add_top_conf("GENERAL", c_general);
	add_top_conf("DATABASE", c_database);
	add_top_conf("LOADMODULE", c_loadmodule);
	add_top_conf("OPERCLASS", c_operclass);
	add_top_conf("OPERATOR", c_operator);
	add_top_conf("LANGUAGE", c_language);
	add_top_conf("STRING", c_string);

	/* Now we fill in the information */
	add_conf_item("NAME", &conf_si_table, c_si_name);
	add_conf_item("DESC", &conf_si_table, c_si_desc);
	add_conf_item("UPLINK", &conf_si_table, c_si_uplink);
	add_conf_item("NUMERIC", &conf_si_table, c_si_numeric);
	add_conf_item("VHOST", &conf_si_table, c_si_vhost);
	add_conf_item("RECONTIME", &conf_si_table, c_si_recontime);
	add_conf_item("RESTARTTIME", &conf_si_table, c_si_restarttime);
	add_conf_item("EXPIRE", &conf_si_table, c_gi_expire);
	add_conf_item("NETNAME", &conf_si_table, c_si_netname);
	add_conf_item("HIDEHOSTSUFFIX", &conf_si_table, c_si_hidehostsuffix);
	add_conf_item("ADMINNAME", &conf_si_table, c_si_adminname);
	add_conf_item("ADMINEMAIL", &conf_si_table, c_si_adminemail);
	add_conf_item("MTA", &conf_si_table, c_si_mta);
	add_conf_item("LOGLEVEL", &conf_si_table, c_si_loglevel);
	add_conf_item("MAXLOGINS", &conf_si_table, c_si_maxlogins);
	add_conf_item("MAXUSERS", &conf_si_table, c_si_maxusers);
	add_conf_item("MAXCHANS", &conf_si_table, c_si_maxchans);
	add_conf_item("EMAILLIMIT", &conf_si_table, c_si_emaillimit);
	add_conf_item("EMAILTIME", &conf_si_table, c_si_emailtime);
	add_conf_item("AUTH", &conf_si_table, c_si_auth);
	add_conf_item("MDLIMIT", &conf_si_table, c_si_mdlimit);
	add_conf_item("CASEMAPPING", &conf_si_table, c_si_casemapping);

	/* general{} block. */
	add_conf_item("CHAN", &conf_gi_table, c_gi_chan);
	add_conf_item("VERBOSE_WALLOPS", &conf_gi_table, c_gi_verbose_wallops);
	add_conf_item("USE_PRIVMSG", &conf_gi_table, c_gi_use_privmsg);
	add_conf_item("SILENT", &conf_gi_table, c_gi_silent);
	add_conf_item("JOIN_CHANS", &conf_gi_table, c_gi_join_chans);
	add_conf_item("LEAVE_CHANS", &conf_gi_table, c_gi_leave_chans);
	add_conf_item("UFLAGS", &conf_gi_table, c_gi_uflags);
	add_conf_item("CFLAGS", &conf_gi_table, c_gi_cflags);
	add_conf_item("RAW", &conf_gi_table, c_gi_raw);
	add_conf_item("SECURE", &conf_gi_table, c_gi_secure);
	add_conf_item("FLOOD_MSGS", &conf_gi_table, c_gi_flood_msgs);
	add_conf_item("FLOOD_TIME", &conf_gi_table, c_gi_flood_time);
	add_conf_item("KLINE_TIME", &conf_gi_table, c_gi_kline_time);
	add_conf_item("COMMIT_INTERVAL", &conf_gi_table, c_gi_commit_interval);
	add_conf_item("EXPIRE", &conf_gi_table, c_gi_expire);
	add_conf_item("SRAS", &conf_gi_table, c_gi_sras);

	/* chanserv{} block */
	add_conf_item("NICK", &conf_ci_table, c_ci_nick);
	add_conf_item("USER", &conf_ci_table, c_ci_user);
	add_conf_item("HOST", &conf_ci_table, c_ci_host);
	add_conf_item("REAL", &conf_ci_table, c_ci_real);
	add_conf_item("FANTASY", &conf_ci_table, c_ci_fantasy);
	add_conf_item("VOP", &conf_ci_table, c_ci_vop);
	add_conf_item("HOP", &conf_ci_table, c_ci_hop);
	add_conf_item("AOP", &conf_ci_table, c_ci_aop);
	add_conf_item("SOP", &conf_ci_table, c_ci_sop);
	add_conf_item("CHANGETS", &conf_ci_table, c_ci_changets);

	/* global{} block */
	add_conf_item("NICK", &conf_gl_table, c_gl_nick);
	add_conf_item("USER", &conf_gl_table, c_gl_user);
	add_conf_item("HOST", &conf_gl_table, c_gl_host);
	add_conf_item("REAL", &conf_gl_table, c_gl_real);

	/* operserv{} block */
	add_conf_item("NICK", &conf_oi_table, c_oi_nick);
	add_conf_item("USER", &conf_oi_table, c_oi_user);
	add_conf_item("HOST", &conf_oi_table, c_oi_host);
	add_conf_item("REAL", &conf_oi_table, c_oi_real);

	/* nickserv{} block */
	add_conf_item("NICK", &conf_ni_table, c_ni_nick);
	add_conf_item("USER", &conf_ni_table, c_ni_user);
	add_conf_item("HOST", &conf_ni_table, c_ni_host);
	add_conf_item("REAL", &conf_ni_table, c_ni_real);
	add_conf_item("SPAM", &conf_ni_table, c_ni_spam);

	/* userserv{} block */
	add_conf_item("NICK", &conf_ui_table, c_ui_nick);
	add_conf_item("USER", &conf_ui_table, c_ui_user);
	add_conf_item("HOST", &conf_ui_table, c_ui_host);
	add_conf_item("REAL", &conf_ui_table, c_ui_real);

	/* saslserv{} block */
	add_conf_item("NICK", &conf_ss_table, c_ss_nick);
	add_conf_item("USER", &conf_ss_table, c_ss_user);
	add_conf_item("HOST", &conf_ss_table, c_ss_host);
	add_conf_item("REAL", &conf_ss_table, c_ss_real);

	/* memoserv{} block */
	add_conf_item("NICK", &conf_ms_table, c_ms_nick);
	add_conf_item("USER", &conf_ms_table, c_ms_user);
	add_conf_item("HOST", &conf_ms_table, c_ms_host);
	add_conf_item("REAL", &conf_ms_table, c_ms_real);
	
	/* memoserv{} block */
	add_conf_item("NICK", &conf_hs_table, c_hs_nick);
	add_conf_item("USER", &conf_hs_table, c_hs_user);
	add_conf_item("HOST", &conf_hs_table, c_hs_host);
	add_conf_item("REAL", &conf_hs_table, c_hs_real);

	/* database{} block */
	add_conf_item("USER", &conf_db_table, c_db_user);
	add_conf_item("HOST", &conf_db_table, c_db_host);
	add_conf_item("PASSWORD", &conf_db_table, c_db_password);
	add_conf_item("DATABASE", &conf_db_table, c_db_database);
	add_conf_item("PORT", &conf_db_table, c_db_port);

	/* language:: stuff */
	add_conf_item("NAME", &conf_la_table, c_la_name);
	add_conf_item("TRANSLATOR", &conf_la_table, c_la_translator);
}

static int c_serverinfo(CONFIGENTRY *ce)
{
	subblock_handler(ce, &conf_si_table);
	return 0;
}

static int c_cservice(CONFIGENTRY *ce)
{
	subblock_handler(ce, &conf_ci_table);
	return 0;
}

static int c_gservice(CONFIGENTRY *ce)
{
	subblock_handler(ce, &conf_gl_table);
	return 0;
}

static int c_oservice(CONFIGENTRY *ce)
{
	subblock_handler(ce, &conf_oi_table);
	return 0;
}

static int c_nickserv(CONFIGENTRY *ce)
{
	subblock_handler(ce, &conf_ni_table);
	return 0;
}

static int c_userserv(CONFIGENTRY *ce)
{
	subblock_handler(ce, &conf_ui_table);
	return 0;
}

static int c_saslserv(CONFIGENTRY *ce)
{
	subblock_handler(ce, &conf_ss_table);
	return 0;
}

static int c_memoserv(CONFIGENTRY *ce)
{
	subblock_handler(ce, &conf_ms_table);
	return 0;
}

static int c_helpserv(CONFIGENTRY *ce)
{
	subblock_handler(ce, &conf_hs_table);
	return 0;
}

static int c_database(CONFIGENTRY *ce)
{
	subblock_handler(ce, &conf_db_table);
	return 0;
}

static int c_loadmodule(CONFIGENTRY *ce)
{
	char pathbuf[4096];
	char *name;

	if (cold_start == FALSE)
		return 0;

	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	name = ce->ce_vardata;

	if (*name == '/')
	{
		module_load(name);
		return 0;
	}
	else
	{
		snprintf(pathbuf, 4096, "%s/%s", MODDIR, name);
		module_load(pathbuf);
		return 0;
	}
}

static int c_uplink(CONFIGENTRY *ce)
{
	char *name;
	char *host = NULL, *vhost = NULL, *password = NULL;
	uint32_t port = 0;

	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	name = sstrdup(ce->ce_vardata);

	for (ce = ce->ce_entries; ce; ce = ce->ce_next)
	{
		if (!strcasecmp("HOST", ce->ce_varname))
		{
			if (ce->ce_vardata == NULL)
				PARAM_ERROR(ce);

			host = sstrdup(ce->ce_vardata);
		}
		else if (!strcasecmp("VHOST", ce->ce_varname))
		{
			if (ce->ce_vardata == NULL)
				PARAM_ERROR(ce);

			vhost = sstrdup(ce->ce_vardata);
		}
		else if (!strcasecmp("PASSWORD", ce->ce_varname))
		{
			if (ce->ce_vardata == NULL)
				PARAM_ERROR(ce);

			password = sstrdup(ce->ce_vardata);
		}
		else if (!strcasecmp("PORT", ce->ce_varname))
		{
			if (ce->ce_vardata == NULL)
				PARAM_ERROR(ce);

			port = ce->ce_vardatanum;
		}
		else
		{
			slog(LG_ERROR, "%s:%d: Invalid configuration option uplink::%s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_varname);
			continue;
		}
	}

	uplink_add(name, host, password, vhost, port);
	return 0;
}

static int c_operclass(CONFIGENTRY *ce)
{
	char *name;
	char *privs = NULL, *newprivs;

	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	name = ce->ce_vardata;

	for (ce = ce->ce_entries; ce; ce = ce->ce_next)
	{
		if (!strcasecmp("PRIVS", ce->ce_varname))
		{
			if (ce->ce_vardata == NULL && ce->ce_entries == NULL)
				PARAM_ERROR(ce);

			if (ce->ce_entries == NULL)
			{
				if (privs == NULL)
					privs = sstrdup(ce->ce_vardata);
				else
				{
					newprivs = smalloc(strlen(privs) + 1 + strlen(ce->ce_vardata) + 1);
					strcpy(newprivs, privs);
					strcat(newprivs, " ");
					strcat(newprivs, ce->ce_vardata);
					free(privs);
					privs = newprivs;
				}
			}
			else
			{
				CONFIGENTRY *conf_p;
				/*
				 * New definition format for operclasses.
				 *
				 * operclass "sra" {
				 *     privs = {
				 *         special:ircop;
				 *     };
				 * };
				 *
				 * - nenolod
				 */
				for (conf_p = ce->ce_entries; conf_p; conf_p = conf_p->ce_next)
				{
					if (privs == NULL)
						privs = sstrdup(conf_p->ce_varname);
					else
					{
						newprivs = smalloc(strlen(privs) + 1 + strlen(conf_p->ce_varname) + 1);
						strcpy(newprivs, privs);
						strcat(newprivs, " ");
						strcat(newprivs, conf_p->ce_varname);
						free(privs);
						privs = newprivs;
					}
				}
			}
		}
		else
		{
			slog(LG_ERROR, "%s:%d: Invalid configuration option operclass::%s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_varname);
			continue;
		}
	}

	operclass_add(name, privs ? privs : "");
	free(privs);
	return 0;
}

static int c_operator(CONFIGENTRY *ce)
{
	char *name;
	operclass_t *operclass = NULL;
	CONFIGENTRY *topce;

	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	topce = ce;
	name = ce->ce_vardata;

	for (ce = ce->ce_entries; ce; ce = ce->ce_next)
	{
		if (!strcasecmp("OPERCLASS", ce->ce_varname))
		{
			if (ce->ce_vardata == NULL)
				PARAM_ERROR(ce);

			operclass = operclass_find(ce->ce_vardata);
			if (operclass == NULL)
				slog(LG_ERROR, "%s:%d: invalid operclass %s for operator %s",
						ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_vardata, name);
		}
		else
		{
			slog(LG_ERROR, "%s:%d: Invalid configuration option operator::%s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_varname);
			continue;
		}
	}

	if (operclass != NULL)
		soper_add(name, operclass);
	else
		slog(LG_ERROR, "%s:%d: skipping operator %s because of bad/missing parameters",
						topce->ce_fileptr->cf_filename, topce->ce_varlinenum, name);
	return 0;
}

static int c_general(CONFIGENTRY *ce)
{
	subblock_handler(ce, &conf_gi_table);
	return 0;
}

/*
 * Ok. This supports:
 *
 * language {
 *         ...
 * };
 *
 * and for the main config:
 *
 * language "translations/blah.language";
 *
 * to set the languagefile setting. So it's rather weird.
 *    --nenolod
 */
static int c_language(CONFIGENTRY *ce)
{
	if (ce->ce_entries)
	{
		subblock_handler(ce, &conf_la_table);
		return 0;
	}
	else
		config_options.languagefile = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_string(CONFIGENTRY *ce)
{
	char *name, *trans = NULL;
	CONFIGENTRY *topce;

	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	topce = ce;
	name = ce->ce_vardata;

	for (ce = ce->ce_entries; ce; ce = ce->ce_next)
	{
		if (!strcasecmp("TRANSLATION", ce->ce_varname))
		{
			if (ce->ce_vardata == NULL)
				PARAM_ERROR(ce);

			trans = ce->ce_vardata;
		}
		else
		{
			slog(LG_ERROR, "%s:%d: Invalid configuration option string::%s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_varname);
			continue;
		}
	}

	if (trans != NULL)
		translation_create(name, trans);
	else
		slog(LG_ERROR, "%s:%d: missing translation for string", topce->ce_fileptr->cf_filename, topce->ce_varlinenum);
	return 0;
}

static int c_la_name(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.language_name = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_la_translator(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.language_translator = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_name(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (!(runflags & RF_REHASHING))
		me.name = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_desc(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.desc = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_uplink(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.uplink = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_numeric(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (!(runflags & RF_REHASHING))
		me.numeric = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_mdlimit(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.mdlimit = ce->ce_vardatanum;

	return 0;
}

static int c_si_vhost(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.vhost = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_recontime(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.recontime = ce->ce_vardatanum;

	return 0;
}

static int c_si_restarttime(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.restarttime = ce->ce_vardatanum;

	return 0;
}

static int c_si_netname(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.netname = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_hidehostsuffix(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.hidehostsuffix = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_adminname(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.adminname = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_adminemail(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.adminemail = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_mta(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.mta = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_loglevel(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (!strcasecmp("DEBUG", ce->ce_vardata))
		me.loglevel |= LG_DEBUG;

	else if (!strcasecmp("ERROR", ce->ce_vardata))
		me.loglevel |= LG_ERROR;

	else if (!strcasecmp("INFO", ce->ce_vardata))
		me.loglevel |= LG_INFO;

	else if (!strcasecmp("NONE", ce->ce_vardata))
		me.loglevel |= LG_NONE;

	else
		me.loglevel |= LG_ERROR;

	return 0;
}

static int c_si_maxlogins(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.maxlogins = ce->ce_vardatanum;

	return 0;

}

static int c_si_maxusers(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.maxusers = ce->ce_vardatanum;

	return 0;

}

static int c_si_maxchans(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.maxchans = ce->ce_vardatanum;

	return 0;
}

static int c_si_emaillimit(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.emaillimit = ce->ce_vardatanum;

	return 0;
}

static int c_si_emailtime(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.emailtime = ce->ce_vardatanum;

	return 0;
}

static int c_si_auth(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (!strcasecmp("EMAIL", ce->ce_vardata))
		me.auth = AUTH_EMAIL;

	else
		me.auth = AUTH_NONE;

	return 0;
}

static int c_si_casemapping(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (!strcasecmp("ASCII", ce->ce_vardata))
		set_match_mapping(MATCH_ASCII);

	else
		set_match_mapping(MATCH_RFC1459);

	return 0;
}

static int c_ci_nick(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.nick = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ci_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ci_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ci_real(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.real = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ci_fantasy(CONFIGENTRY *ce)
{
	chansvs.fantasy = TRUE;
	
	if (chansvs.me != NULL)
		fcmd_agent = chansvs.me;

	return 0;
}

static int c_ci_vop(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.ca_vop = flags_to_bitmask(ce->ce_vardata, chanacs_flags, 0);

	return 0;
}

static int c_ci_hop(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.ca_hop = flags_to_bitmask(ce->ce_vardata, chanacs_flags, 0);

	return 0;
}

static int c_ci_aop(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.ca_aop = flags_to_bitmask(ce->ce_vardata, chanacs_flags, 0);

	return 0;
}

static int c_ci_sop(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.ca_sop = flags_to_bitmask(ce->ce_vardata, chanacs_flags, 0);

	return 0;
}

static int c_ci_changets(CONFIGENTRY *ce)
{
	chansvs.changets = TRUE;
	return 0;
}

static int c_gi_chan(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.chan = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gi_silent(CONFIGENTRY *ce)
{
	config_options.silent = TRUE;
	return 0;
}

static int c_gi_verbose_wallops(CONFIGENTRY *ce)
{
	config_options.verbose_wallops = TRUE;
	return 0;
}

static int c_gi_use_privmsg(CONFIGENTRY *ce)
{
	config_options.use_privmsg = TRUE;
	return 0;
}

static int c_gi_secure(CONFIGENTRY *ce)
{
	config_options.secure = TRUE;
	return 0;
}

static int c_gi_join_chans(CONFIGENTRY *ce)
{
	config_options.join_chans = TRUE;
	return 0;
}

static int c_gi_leave_chans(CONFIGENTRY *ce)
{
	config_options.leave_chans = TRUE;
	return 0;
}

static int c_gi_uflags(CONFIGENTRY *ce)
{
	CONFIGENTRY *flce;

	for (flce = ce->ce_entries; flce; flce = flce->ce_next)
	{
		int val;

		val = token_to_value(uflags, flce->ce_varname);

		if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
			config_options.defuflags |= val;

		else
		{
			slog(LG_INFO, "%s:%d: unknown flag: %s", flce->ce_fileptr->cf_filename, flce->ce_varlinenum, flce->ce_varname);
		}
	}

	return 0;
}

static int c_gi_cflags(CONFIGENTRY *ce)
{
	CONFIGENTRY *flce;

	for (flce = ce->ce_entries; flce; flce = flce->ce_next)
	{
		int val;

		val = token_to_value(cflags, flce->ce_varname);

		if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
			config_options.defcflags |= val;

		else
		{
			slog(LG_INFO, "%s:%d: unknown flag: %s", flce->ce_fileptr->cf_filename, flce->ce_varlinenum, flce->ce_varname);
		}
	}

	return 0;
}

static int c_gi_raw(CONFIGENTRY *ce)
{
	config_options.raw = TRUE;
	return 0;
}

static int c_gi_flood_msgs(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.flood_msgs = ce->ce_vardatanum;

	return 0;
}

static int c_gi_flood_time(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.flood_time = ce->ce_vardatanum;

	return 0;
}

static int c_gi_kline_time(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.kline_time = (ce->ce_vardatanum * 60 * 60 * 24);

	return 0;
}

static int c_gi_commit_interval(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.commit_interval = (ce->ce_vardatanum * 60);

	return 0;
}

static int c_gi_expire(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.expire = (ce->ce_vardatanum * 60 * 60 * 24);

	return 0;
}

static int c_oi_nick(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	opersvs.nick = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_oi_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	opersvs.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_oi_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	opersvs.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_oi_real(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	opersvs.real = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ni_nick(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	nicksvs.nick = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ni_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	nicksvs.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ni_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	nicksvs.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ni_real(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	nicksvs.real = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ni_spam(CONFIGENTRY *ce)
{
	nicksvs.spam = TRUE;
	return 0;
}

static int c_ui_nick(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	usersvs.nick = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ui_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	usersvs.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ui_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	usersvs.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ui_real(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	usersvs.real = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ss_nick(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	saslsvs.nick = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ss_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	saslsvs.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ss_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	saslsvs.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ss_real(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	saslsvs.real = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ms_nick(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	memosvs.nick = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ms_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	memosvs.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ms_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	memosvs.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ms_real(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	memosvs.real = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_hs_nick(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	helpsvs.nick = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_hs_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	helpsvs.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_hs_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	helpsvs.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_hs_real(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	helpsvs.real = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gl_nick(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	globsvs.nick = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gl_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	globsvs.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gl_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	globsvs.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gl_real(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	globsvs.real = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gi_sras(CONFIGENTRY *ce)
{
	CONFIGENTRY *flce;

	for (flce = ce->ce_entries; flce; flce = flce->ce_next)
		soper_add(flce->ce_varname, NULL);

	return 0;
}

static int c_db_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	database_options.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_db_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	database_options.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_db_password(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	database_options.pass = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_db_database(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	database_options.database = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_db_port(CONFIGENTRY *ce)
{
	if (ce->ce_vardatanum == 0)
		PARAM_ERROR(ce);

	database_options.port = ce->ce_vardatanum;

	return 0;
}

static void copy_me(struct me *src, struct me *dst)
{
	dst->recontime = src->recontime;
	dst->restarttime = src->restarttime;
	dst->netname = sstrdup(src->netname);
	dst->hidehostsuffix = sstrdup(src->hidehostsuffix);
	dst->adminname = sstrdup(src->adminname);
	dst->adminemail = sstrdup(src->adminemail);
	dst->mta = src->mta ? sstrdup(src->mta) : NULL;
	dst->loglevel = src->loglevel;
	dst->maxlogins = src->maxlogins;
	dst->maxusers = src->maxusers;
	dst->maxchans = src->maxchans;
	dst->emaillimit = src->emaillimit;
	dst->emailtime = src->emailtime;
	dst->auth = src->auth;
}

static void free_cstructs(struct me *mesrc, chansvs_t *svssrc)
{
	free(mesrc->netname);
	free(mesrc->hidehostsuffix);
	free(mesrc->adminname);
	free(mesrc->adminemail);
	free(mesrc->mta);

	free(svssrc->nick);
}

boolean_t conf_rehash(void)
{
	struct me *hold_me = scalloc(sizeof(struct me), 1);	/* and keep_me_warm; */
	char *oldsnoop;
	CONFIGFILE *cfp;

	/* we're rehashing */
	slog(LG_INFO, "conf_rehash(): rehashing");
	runflags |= RF_REHASHING;

	errno = 0;
	cfp = config_load(config_file);
	if (cfp == NULL)
	{
		slog(LG_INFO, "conf_rehash(): unable to load configuration file: %s, aborting rehash", strerror(errno));
		runflags &= ~RF_REHASHING;
		return FALSE;
	}

	copy_me(&me, hold_me);

	oldsnoop = config_options.chan != NULL ? sstrdup(config_options.chan) :
		NULL;

	/* reset everything */
	conf_init();

	mark_all_illegal();

	/* now reload */
	conf_process(cfp);
	config_free(cfp);
	hook_call_event("config_ready", NULL);

	/* now recheck */
	if (!conf_check())
	{
		slog(LG_INFO, "conf_rehash(): conf file was malformed, aborting rehash");

		/* freeing the new conf strings */
		free_cstructs(&me, &chansvs);

		/* return everything to the way it was before */
		copy_me(hold_me, &me);

		/* not fully ok, oh well */
		unmark_all_illegal();

		free(hold_me);
		free(oldsnoop);

		runflags &= ~RF_REHASHING;
		return FALSE;
	}

	if (oldsnoop != NULL || config_options.chan != NULL)
	{
		if (config_options.chan == NULL)
			partall(oldsnoop);
		else if (oldsnoop == NULL)
			joinall(config_options.chan);
		else if (strcmp(oldsnoop, config_options.chan))
		{
			partall(oldsnoop);
			joinall(config_options.chan);
		}
	}

	remove_illegals();

	free(hold_me);
	free(oldsnoop);

	runflags &= ~RF_REHASHING;
	return TRUE;
}

boolean_t conf_check(void)
{
	if (!me.name)
	{
		slog(LG_INFO, "conf_check(): no `name' set in %s", config_file);
		return FALSE;
	}

	if (!strchr(me.name, '.'))
	{
		slog(LG_INFO, "conf_check(): bogus `name' in %s (did you specify a valid server name?)", config_file);
		return FALSE;
	}

	if (!me.desc)
		me.desc = sstrdup("Atheme IRC Services");

	if ((!me.recontime) || (me.recontime < 10))
	{
		slog(LG_INFO, "conf_check(): invalid `recontime' set in %s; " "defaulting to 10", config_file);
		me.recontime = 10;
	}

	if (!me.netname)
	{
		slog(LG_INFO, "conf_check(): no `netname' set in %s", config_file);
		return FALSE;
	}

	if (!me.adminname)
	{
		slog(LG_INFO, "conf_check(): no `adminname' set in %s", config_file);
		return FALSE;
	}

	if (!me.adminemail)
	{
		slog(LG_INFO, "conf_check(): no `adminemail' set in %s", config_file);
		return FALSE;
	}

	if (!me.mta && me.auth == AUTH_EMAIL)
	{
		slog(LG_INFO, "conf_check(): no `mta' set in %s (but `auth' is email)", config_file);
		return FALSE;
	}

	if (!me.maxlogins)
	{
		slog(LG_INFO, "conf_check(): no `maxlogins' set in %s; " "defaulting to 5", config_file);
		me.maxlogins = 5;
	}

	if (!me.maxusers)
	{
		slog(LG_INFO, "conf_check(): no `maxusers' set in %s; " "defaulting to 5", config_file);
		me.maxusers = 5;
	}

	if (!me.maxchans)
	{
		slog(LG_INFO, "conf_check(): no `maxchans' set in %s; " "defaulting to 5", config_file);
		me.maxchans = 5;
	}

	if (!me.emaillimit)
	{
		slog(LG_INFO, "conf_check(): no `emaillimit' set in %s; " "defaulting to 10", config_file);
		me.emaillimit = 10;
	}

	if (!me.emailtime)
	{
		slog(LG_INFO, "conf_check(): no `emailtime' set in %s; " "defaulting to 300", config_file);
		me.emailtime = 300;
	}

	if (me.auth != 0 && me.auth != 1)
	{
		slog(LG_INFO, "conf_check(): no `auth' set in %s; " "defaulting to NONE", config_file);
		me.auth = AUTH_NONE;
	}

	if (!chansvs.nick || !chansvs.user || !chansvs.host || !chansvs.real)
	{
		slog(LG_INFO, "conf_check(): invalid chanserv{} block in %s", config_file);
		return FALSE;
	}

	if ((strchr(chansvs.user, ' ')) || (strlen(chansvs.user) > 10))
	{
		slog(LG_INFO, "conf_check(): invalid `chanserv::user' in %s", config_file);
		return FALSE;
	}

	if (!chansvs.ca_vop || !chansvs.ca_hop || !chansvs.ca_aop ||
			!chansvs.ca_sop || chansvs.ca_vop == chansvs.ca_hop ||
			chansvs.ca_vop == chansvs.ca_aop ||
			chansvs.ca_vop == chansvs.ca_sop ||
			chansvs.ca_hop == chansvs.ca_aop ||
			chansvs.ca_hop == chansvs.ca_sop ||
			chansvs.ca_aop == chansvs.ca_sop)
	{
		slog(LG_INFO, "conf_check(): invalid xop levels in %s, using defaults", config_file);
		chansvs.ca_vop = CA_VOP_DEF;
		chansvs.ca_hop = CA_HOP_DEF;
		chansvs.ca_aop = CA_AOP_DEF;
		chansvs.ca_sop = CA_SOP_DEF;
	}

	if (config_options.flood_msgs && !config_options.flood_time)
		config_options.flood_time = 10;


	/* recall that commit_interval is in seconds */
	if ((!config_options.commit_interval) || (config_options.commit_interval < 60) || (config_options.commit_interval > 3600))
	{
		slog(LG_INFO, "conf_check(): invalid `commit_interval' set in %s; " "defaulting to 5 minutes", config_file);
		config_options.commit_interval = 300;
	}

	return TRUE;
}
