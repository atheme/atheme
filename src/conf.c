/*
 * atheme-services: A collection of minimalist IRC services   
 * conf.c: Configuration processing.
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
#include "pmodule.h"
#include "privs.h"
#include "datastream.h"

enum conftype
{
	CONF_HANDLER,
	CONF_UINT,
	CONF_DUPSTR,
	CONF_BOOL
};

struct ConfTable
{
	char *name;
	enum conftype type;
	union
	{
		int (*handler) (config_entry_t *);
		struct
		{
			unsigned int *var;
			unsigned int min;
			unsigned int max;
		} uint_val;
		char **dupstr_val;
		bool *bool_val;
	} un;
};

static inline int
PARAM_ERROR(config_entry_t *ce)
{
	slog(LG_INFO, "%s:%i: no parameter for configuration option: %s",
	     ce->ce_fileptr->cf_filename,
	     ce->ce_varlinenum,
	     ce->ce_varname);
	return 1;
}

static int c_serverinfo(config_entry_t *);
static int c_cservice(config_entry_t *);
static int c_general(config_entry_t *);
static int c_uplink(config_entry_t *);
static int c_nickserv(config_entry_t *);
static int c_loadmodule(config_entry_t *);
static int c_operclass(config_entry_t *);
static int c_operator(config_entry_t *);
static int c_language(config_entry_t *);
static int c_string(config_entry_t *);
static int c_logfile(config_entry_t *);

static int c_si_name(config_entry_t *);
static int c_si_desc(config_entry_t *);
static int c_si_numeric(config_entry_t *);
static int c_si_vhost(config_entry_t *);
static int c_si_recontime(config_entry_t *);
static int c_si_restarttime(config_entry_t *);
static int c_si_netname(config_entry_t *);
static int c_si_hidehostsuffix(config_entry_t *);
static int c_si_adminname(config_entry_t *);
static int c_si_adminemail(config_entry_t *);
static int c_si_mta(config_entry_t *);
static int c_si_loglevel(config_entry_t *);
static int c_si_maxlogins(config_entry_t *);
static int c_si_maxusers(config_entry_t *);
static int c_si_maxnicks(config_entry_t *);
static int c_si_maxchans(config_entry_t *);
static int c_si_emaillimit(config_entry_t *);
static int c_si_emailtime(config_entry_t *);
static int c_si_auth(config_entry_t *);
static int c_si_mdlimit(config_entry_t *);
static int c_si_casemapping(config_entry_t *);

/* CService client information. */
static int c_ci_vop(config_entry_t *);
static int c_ci_hop(config_entry_t *);
static int c_ci_aop(config_entry_t *);
static int c_ci_sop(config_entry_t *);
static int c_ci_trigger(config_entry_t *);
static int c_ci_expire(config_entry_t *);
static int c_ci_maxchanacs(config_entry_t *);
static int c_ci_maxfounders(config_entry_t *);
static int c_ci_deftemplates(config_entry_t *);

/* NickServ client information. */
static int c_ni_expire(config_entry_t *);
static int c_ni_enforce_expire(config_entry_t *);
static int c_ni_enforce_delay(config_entry_t *);

/* language:: stuff */
static int c_la_name(config_entry_t *);
static int c_la_translator(config_entry_t *);

static int c_gi_chan(config_entry_t *);
static int c_gi_uflags(config_entry_t *);
static int c_gi_cflags(config_entry_t *);
static int c_gi_flood_msgs(config_entry_t *);
static int c_gi_flood_time(config_entry_t *);
static int c_gi_kline_time(config_entry_t *);
static int c_gi_commit_interval(config_entry_t *);
static int c_gi_expire(config_entry_t *);
static int c_gi_default_clone_limit(config_entry_t *);
static int c_gi_uplink_sendq_limit(config_entry_t *);

static BlockHeap *conftable_heap;

/* *INDENT-OFF* */

static struct Token uflags[] = {
  { "HOLD",      MU_HOLD         },
  { "NEVEROP",   MU_NEVEROP      },
  { "NOOP",      MU_NOOP         },
  { "HIDEMAIL",  MU_HIDEMAIL     },
  { "NOMEMO",    MU_NOMEMO       },
  { "ENFORCE",   MU_ENFORCE      },
  { "PRIVMSG",   MU_USE_PRIVMSG  },
  { "PRIVATE",   MU_PRIVATE      },
  { "QUIETCHG",  MU_QUIETCHG     },
  { "NONE",      0               },
  { NULL, 0 }
};

static struct Token cflags[] = {
  { "HOLD",        MC_HOLD        },
  { "SECURE",      MC_SECURE      },
  { "VERBOSE",     MC_VERBOSE     },
  { "KEEPTOPIC",   MC_KEEPTOPIC   },
  { "VERBOSE_OPS", MC_VERBOSE_OPS },
  { "TOPICLOCK",   MC_TOPICLOCK   },
  { "GUARD",	   MC_GUARD	  },
  { "NONE",        0              },
  { NULL, 0 }
};

static struct Token logflags[] = {
  { "DEBUG",       LG_ALL                                                                                     },
  { "TRACE",       LG_INFO | LG_ERROR | LG_CMD_ALL | LG_NETWORK | LG_WALLOPS | LG_REGISTER                    },
  { "MISC",        LG_INFO | LG_ERROR | LG_CMD_ADMIN | LG_CMD_REGISTER | LG_CMD_SET | LG_NETWORK | LG_WALLOPS | LG_REGISTER },
  { "NOTICE",      LG_INFO | LG_ERROR | LG_CMD_ADMIN | LG_CMD_REGISTER | LG_NETWORK | LG_REGISTER             },
  { "ALL",         LG_ALL                                                                                     },
  { "INFO",        LG_INFO                                                                                    },
  { "ERROR",       LG_ERROR                                                                                   },
  { "COMMANDS",    LG_CMD_ALL                                                                                 },
  { "ADMIN",       LG_CMD_ADMIN                                                                               },
  { "REGISTER",    LG_CMD_REGISTER | LG_REGISTER                                                              },
  { "SET",         LG_CMD_SET                                                                                 },
  { "NETWORK",     LG_NETWORK                                                                                 },
  { "WALLOPS",     LG_WALLOPS                                                                                 },
  { "RAWDATA",     LG_RAWDATA                                                                                 },
  { NULL,          0                                                                                          }
};

list_t confblocks;
list_t conf_si_table;
list_t conf_ci_table;
list_t conf_ni_table;
list_t conf_gi_table;
list_t conf_ms_table;
list_t conf_la_table;

/* *INDENT-ON* */

static void conf_report_error(config_entry_t *ce, const char *fmt, ...)
{
	va_list va;
	char buf[BUFSIZE];

	return_if_fail(ce != NULL);
	return_if_fail(fmt != NULL);

	va_start(va, fmt);
	vsnprintf(buf, BUFSIZE, fmt, va);
	va_end(va);

	slog(LG_INFO, "%s:%d: configuration error - %s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, buf);
}

static void process_configentry(struct ConfTable *ct, config_entry_t *ce)
{
	unsigned long v;
	char *end;

	switch (ct->type)
	{
		case CONF_HANDLER:
			ct->un.handler(ce);
			break;
		case CONF_UINT:
			if (ce->ce_vardata == NULL)
			{
				PARAM_ERROR(ce);
				return;
			}
			errno = 0;
			v = strtoul(ce->ce_vardata, &end, 10);
			if (errno != 0 || *end != '\0' || end == ce->ce_vardata)
			{
				slog(LG_INFO, "%s:%i: invalid number \"%s\" for configuration option: %s",
						ce->ce_fileptr->cf_filename,
						ce->ce_varlinenum,
						ce->ce_vardata,
						ce->ce_varname);
				return;
			}
			if (v > ct->un.uint_val.max || v < ct->un.uint_val.min)
			{
				slog(LG_INFO, "%s:%i: value %lu is out of range [%u,%u] for configuration option: %s",
						ce->ce_fileptr->cf_filename,
						ce->ce_varlinenum,
						v,
						ct->un.uint_val.min,
						ct->un.uint_val.max,
						ce->ce_varname);
				return;
			}
			*ct->un.uint_val.var = v;
			break;
		case CONF_DUPSTR:
			if (ce->ce_vardata == NULL)
			{
				PARAM_ERROR(ce);
				return;
			}
			free(*ct->un.dupstr_val);
			*ct->un.dupstr_val = sstrdup(ce->ce_vardata);
			break;
		case CONF_BOOL:
			if (ce->ce_vardata == NULL ||
					!strcasecmp(ce->ce_vardata, "yes") ||
					!strcasecmp(ce->ce_vardata, "on") ||
					!strcasecmp(ce->ce_vardata, "true") ||
					!strcmp(ce->ce_vardata, "1"))
				*ct->un.bool_val = true;
			else if (!strcasecmp(ce->ce_vardata, "no") ||
					!strcasecmp(ce->ce_vardata, "off") ||
					!strcasecmp(ce->ce_vardata, "false") ||
					!strcmp(ce->ce_vardata, "0"))
				*ct->un.bool_val = false;
			else
			{
				slog(LG_INFO, "%s:%i: invalid boolean \"%s\" for configuration option: %s, assuming true for now",
						ce->ce_fileptr->cf_filename,
						ce->ce_varlinenum,
						ce->ce_vardata,
						ce->ce_varname);
				*ct->un.bool_val = true;
				return;
			}
	}
}

static void conf_process(config_file_t *cfp)
{
	config_file_t *cfptr;
	config_entry_t *ce;
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
					process_configentry(ct, ce);
					break;
				}
			}

			if (ct == NULL)
				conf_report_error(ce, "invalid configuration option: %s", ce->ce_varname);
		}
	}
}

bool conf_parse(const char *file)
{
	config_file_t *cfp;

	cfp = config_load(file);
	if (cfp == NULL)
		return false;

	conf_process(cfp);
	config_free(cfp);

	if (!pmodule_loaded)
	{
		slog(LG_ERROR, "No protocol module loaded, aborting");
		exit(EXIT_FAILURE);
	}

	hook_call_event("config_ready", NULL);
	return true;
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
	if (config_options.chan)
		free(config_options.chan);
	if (config_options.global)
		free(config_options.global);
	if (config_options.languagefile)
		free(config_options.languagefile);

	me.netname = me.hidehostsuffix = me.adminname = me.adminemail = me.mta = config_options.chan = 
		config_options.global = config_options.languagefile = NULL;

	me.recontime = me.restarttime = me.maxlogins = me.maxusers = me.maxnicks = me.maxchans = me.emaillimit = me.emailtime = 
		config_options.flood_msgs = config_options.flood_time = config_options.kline_time = config_options.commit_interval = config_options.default_clone_limit = 0;

	config_options.uplink_sendq_limit = 1048576;

	nicksvs.expiry = nicksvs.enforce_expiry = chansvs.expiry = 0;
	nicksvs.enforce_delay = 30;

	config_options.defuflags = config_options.defcflags = 0x00000000;

	config_options.silent = config_options.join_chans = config_options.leave_chans = config_options.raw = false;

	me.auth = AUTH_NONE;

	me.mdlimit = 30;

	chansvs.fantasy = false;
	chansvs.ca_vop = CA_VOP_DEF & ca_all;
	chansvs.ca_hop = CA_HOP_DEF & ca_all;
	chansvs.ca_aop = CA_AOP_DEF & ca_all;
	chansvs.ca_sop = CA_SOP_DEF & ca_all;
	chansvs.changets = false;
	if (chansvs.trigger != NULL)
		free(chansvs.trigger);
	chansvs.trigger = sstrdup("!");
	chansvs.maxchanacs = 0;
	chansvs.maxfounders = 4;
	if (chansvs.deftemplates != NULL)
		free(chansvs.deftemplates);
	chansvs.deftemplates = NULL;

	if (!(runflags & RF_REHASHING))
	{
		if (me.name)
			free(me.name);
		if (me.desc)
			free(me.desc);
		if (me.vhost)
			free(me.vhost);

		me.name = me.desc = me.vhost = NULL;

		set_match_mapping(MATCH_RFC1459);	/* default to RFC compliancy */
	}
}

int subblock_handler(config_entry_t *ce, list_t *entries)
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
				process_configentry(ct, ce);
				break;
			}
		}

		if (ct == NULL)
			conf_report_error(ce, "invalid configuration option: %s", ce->ce_varname);
	}
	return 0;
}

struct ConfTable *find_top_conf(const char *name)
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

struct ConfTable *find_conf_item(const char *name, list_t *conflist)
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

void add_top_conf(const char *name, int (*handler) (config_entry_t *ce))
{
	struct ConfTable *ct;

	if ((ct = find_top_conf(name)))
	{
		slog(LG_DEBUG, "add_top_conf(): duplicate config block '%s'.", name);
		return;
	}

	ct = BlockHeapAlloc(conftable_heap);

	ct->name = sstrdup(name);
	ct->type = CONF_HANDLER;
	ct->un.handler = handler;

	node_add(ct, node_create(), &confblocks);
}

void add_conf_item(const char *name, list_t *conflist, int (*handler) (config_entry_t *ce))
{
	struct ConfTable *ct;

	if ((ct = find_conf_item(name, conflist)))
	{
		slog(LG_DEBUG, "add_conf_item(): duplicate item %s", name);
		return;
	}

	ct = BlockHeapAlloc(conftable_heap);

	ct->name = sstrdup(name);
	ct->type = CONF_HANDLER;
	ct->un.handler = handler;

	node_add(ct, node_create(), conflist);
}

void add_uint_conf_item(const char *name, list_t *conflist, unsigned int *var, unsigned int min, unsigned int max)
{
	struct ConfTable *ct;

	if ((ct = find_conf_item(name, conflist)))
	{
		slog(LG_DEBUG, "add_uint_conf_item(): duplicate item %s", name);
		return;
	}

	ct = BlockHeapAlloc(conftable_heap);

	ct->name = sstrdup(name);
	ct->type = CONF_UINT;
	ct->un.uint_val.var = var;
	ct->un.uint_val.min = min;
	ct->un.uint_val.max = max;

	node_add(ct, node_create(), conflist);
}

void add_dupstr_conf_item(const char *name, list_t *conflist, char **var)
{
	struct ConfTable *ct;

	if ((ct = find_conf_item(name, conflist)))
	{
		slog(LG_DEBUG, "add_dupstr_conf_item(): duplicate item %s", name);
		return;
	}

	ct = BlockHeapAlloc(conftable_heap);

	ct->name = sstrdup(name);
	ct->type = CONF_DUPSTR;
	ct->un.dupstr_val = var;

	node_add(ct, node_create(), conflist);
}

void add_bool_conf_item(const char *name, list_t *conflist, bool *var)
{
	struct ConfTable *ct;

	if ((ct = find_conf_item(name, conflist)))
	{
		slog(LG_DEBUG, "add_bool_conf_item(): duplicate item %s", name);
		return;
	}

	ct = BlockHeapAlloc(conftable_heap);

	ct->name = sstrdup(name);
	ct->type = CONF_BOOL;
	ct->un.bool_val = var;

	node_add(ct, node_create(), conflist);
}

void del_top_conf(const char *name)
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

void del_conf_item(const char *name, list_t *conflist)
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

conf_handler_t conftable_get_conf_handler(struct ConfTable *ct)
{
	return ct->type == CONF_HANDLER ? ct->un.handler : NULL;
}

/* stolen from Sentinel */
int token_to_value(struct Token token_table[], const char *token)
{
	int i;

	if ((token_table != NULL) && (token != NULL))
	{
		for (i = 0; token_table[i].text != NULL; i++)
		{
			if (strcasecmp(token_table[i].text, token) == 0)
			{
				return token_table[i].value;
			}
		}
		/* If no match... */
		return TOKEN_UNMATCHED;
	}

	/* Otherwise... */
	return TOKEN_ERROR;
}

void init_newconf(void)
{
	conftable_heap = BlockHeapCreate(sizeof(struct ConfTable), 32);

	if (!conftable_heap)
	{
		slog(LG_ERROR, "init_newconf(): block allocator failure.");
		exit(EXIT_FAILURE);
	}

	/* First we set up the blocks. */
	add_top_conf("SERVERINFO", c_serverinfo);
	add_top_conf("CHANSERV", c_cservice);
	add_top_conf("NICKSERV", c_nickserv);
	add_top_conf("UPLINK", c_uplink);
	add_top_conf("GENERAL", c_general);
	add_top_conf("LOADMODULE", c_loadmodule);
	add_top_conf("OPERCLASS", c_operclass);
	add_top_conf("OPERATOR", c_operator);
	add_top_conf("LANGUAGE", c_language);
	add_top_conf("STRING", c_string);
	add_top_conf("LOGFILE", c_logfile);

	/* Now we fill in the information */
	add_conf_item("NAME", &conf_si_table, c_si_name);
	add_conf_item("DESC", &conf_si_table, c_si_desc);
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
	add_conf_item("MAXNICKS", &conf_si_table, c_si_maxnicks);
	add_conf_item("MAXCHANS", &conf_si_table, c_si_maxchans);
	add_conf_item("EMAILLIMIT", &conf_si_table, c_si_emaillimit);
	add_conf_item("EMAILTIME", &conf_si_table, c_si_emailtime);
	add_conf_item("AUTH", &conf_si_table, c_si_auth);
	add_conf_item("MDLIMIT", &conf_si_table, c_si_mdlimit);
	add_conf_item("CASEMAPPING", &conf_si_table, c_si_casemapping);

	/* general{} block. */
	add_conf_item("CHAN", &conf_gi_table, c_gi_chan);
	add_bool_conf_item("VERBOSE_WALLOPS", &conf_gi_table, &config_options.verbose_wallops);
	add_bool_conf_item("SILENT", &conf_gi_table, &config_options.silent);
	add_bool_conf_item("JOIN_CHANS", &conf_gi_table, &config_options.join_chans);
	add_bool_conf_item("LEAVE_CHANS", &conf_gi_table, &config_options.leave_chans);
	add_conf_item("UFLAGS", &conf_gi_table, c_gi_uflags);
	add_conf_item("CFLAGS", &conf_gi_table, c_gi_cflags);
	add_bool_conf_item("RAW", &conf_gi_table, &config_options.raw);
	add_bool_conf_item("SECURE", &conf_gi_table, &config_options.secure);
	add_conf_item("FLOOD_MSGS", &conf_gi_table, c_gi_flood_msgs);
	add_conf_item("FLOOD_TIME", &conf_gi_table, c_gi_flood_time);
	add_conf_item("KLINE_TIME", &conf_gi_table, c_gi_kline_time);
	add_conf_item("COMMIT_INTERVAL", &conf_gi_table, c_gi_commit_interval);
	add_conf_item("EXPIRE", &conf_gi_table, c_gi_expire);
	add_conf_item("DEFAULT_CLONE_LIMIT", &conf_gi_table, c_gi_default_clone_limit);
	add_conf_item("UPLINK_SENDQ_LIMIT", &conf_gi_table, c_gi_uplink_sendq_limit);

	/* chanserv{} block */
	add_bool_conf_item("FANTASY", &conf_ci_table, &chansvs.fantasy);
	add_conf_item("VOP", &conf_ci_table, c_ci_vop);
	add_conf_item("HOP", &conf_ci_table, c_ci_hop);
	add_conf_item("AOP", &conf_ci_table, c_ci_aop);
	add_conf_item("SOP", &conf_ci_table, c_ci_sop);
	add_bool_conf_item("CHANGETS", &conf_ci_table, &chansvs.changets);
	add_conf_item("TRIGGER", &conf_ci_table, c_ci_trigger);
	add_conf_item("EXPIRE", &conf_ci_table, c_ci_expire);
	add_conf_item("MAXCHANACS", &conf_ci_table, c_ci_maxchanacs);
	add_conf_item("MAXFOUNDERS", &conf_ci_table, c_ci_maxfounders);
	add_conf_item("DEFTEMPLATES", &conf_ci_table, c_ci_deftemplates);

	/* global{} block */

	/* operserv{} block */

	/* nickserv{} block */
	add_bool_conf_item("SPAM", &conf_ni_table, &nicksvs.spam);
	add_bool_conf_item("NO_NICK_OWNERSHIP", &conf_ni_table, &nicksvs.no_nick_ownership);
	add_conf_item("EXPIRE", &conf_ni_table, c_ni_expire);
	add_conf_item("ENFORCE_EXPIRE", &conf_ni_table, c_ni_enforce_expire);
	add_conf_item("ENFORCE_DELAY", &conf_ni_table, c_ni_enforce_delay);

	/* saslserv{} block */

	/* memoserv{} block */

	/* memoserv{} block */
	
	/* language:: stuff */
	add_conf_item("NAME", &conf_la_table, c_la_name);
	add_conf_item("TRANSLATOR", &conf_la_table, c_la_translator);
}

static int c_serverinfo(config_entry_t *ce)
{
	subblock_handler(ce, &conf_si_table);
	return 0;
}

static int c_cservice(config_entry_t *ce)
{
	subblock_handler(ce, &conf_ci_table);
	return 0;
}

static int c_nickserv(config_entry_t *ce)
{
	subblock_handler(ce, &conf_ni_table);
	return 0;
}

static int c_loadmodule(config_entry_t *ce)
{
	char pathbuf[4096];
	char *name;

	if (cold_start == false)
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

static int c_uplink(config_entry_t *ce)
{
	char *name;
	char *host = NULL, *vhost = NULL, *password = NULL;
	unsigned int port = 0;

	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (me.name != NULL && !irccasecmp(ce->ce_vardata, me.name))
		slog(LG_ERROR, "%s:%d: uplink's server name %s should not be the same as our server name, continuing anyway", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_vardata);
	else if (!strchr(ce->ce_vardata, '.'))
		slog(LG_ERROR, "%s:%d: uplink's server name %s is invalid, continuing anyway", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_vardata);
	else if (isdigit(ce->ce_vardata[0]))
		slog(LG_ERROR, "%s:%d: uplink's server name %s starts with a digit, probably invalid (continuing anyway)", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_vardata);

	name = ce->ce_vardata;

	for (ce = ce->ce_entries; ce; ce = ce->ce_next)
	{
		if (!strcasecmp("HOST", ce->ce_varname))
		{
			if (ce->ce_vardata == NULL)
				PARAM_ERROR(ce);

			host = ce->ce_vardata;
		}
		else if (!strcasecmp("VHOST", ce->ce_varname))
		{
			if (ce->ce_vardata == NULL)
				PARAM_ERROR(ce);

			vhost = ce->ce_vardata;
		}
		else if (!strcasecmp("PASSWORD", ce->ce_varname))
		{
			if (ce->ce_vardata == NULL)
				PARAM_ERROR(ce);

			password = ce->ce_vardata;
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

static int c_operclass(config_entry_t *ce)
{
	operclass_t *operclass;
	char *name;
	char *privs = NULL, *newprivs;
	int flags = 0;

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
				config_entry_t *conf_p;
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
		else if (!strcasecmp("NEEDOPER", ce->ce_varname))
			flags |= OPERCLASS_NEEDOPER;
		else
		{
			slog(LG_ERROR, "%s:%d: Invalid configuration option operclass::%s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_varname);
			continue;
		}
	}

	operclass = operclass_add(name, privs ? privs : "");
	if (operclass != NULL)
		operclass->flags |= flags;
	free(privs);
	return 0;
}

static int c_operator(config_entry_t *ce)
{
	char *name;
	char *password = NULL;
	operclass_t *operclass = NULL;
	config_entry_t *topce;

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
		else if (!strcasecmp("PASSWORD", ce->ce_varname))
		{
			if (ce->ce_vardata == NULL)
				PARAM_ERROR(ce);

			password = ce->ce_vardata;
		}
		else
		{
			slog(LG_ERROR, "%s:%d: Invalid configuration option operator::%s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_varname);
			continue;
		}
	}

	if (operclass != NULL)
		soper_add(name, operclass->name, SOPER_CONF, password);
	else
		slog(LG_ERROR, "%s:%d: skipping operator %s because of bad/missing parameters",
						topce->ce_fileptr->cf_filename, topce->ce_varlinenum, name);
	return 0;
}

static int c_general(config_entry_t *ce)
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
static int c_language(config_entry_t *ce)
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

static int c_string(config_entry_t *ce)
{
	char *name, *trans = NULL;
	config_entry_t *topce;

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

static int c_la_name(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.language_name = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_la_translator(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.language_translator = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_name(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (!(runflags & RF_REHASHING))
		me.name = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_desc(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.desc = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_numeric(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (!(runflags & RF_REHASHING))
		me.numeric = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_mdlimit(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.mdlimit = ce->ce_vardatanum;

	return 0;
}

static int c_si_vhost(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.vhost = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_recontime(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.recontime = ce->ce_vardatanum;

	return 0;
}

static int c_si_restarttime(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.restarttime = ce->ce_vardatanum;

	return 0;
}

static int c_si_netname(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.netname = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_hidehostsuffix(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.hidehostsuffix = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_adminname(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.adminname = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_adminemail(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.adminemail = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_mta(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.mta = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_loglevel(config_entry_t *ce)
{
	config_entry_t *flce;
	int val;
	int mask = 0;

	if (ce->ce_vardata != NULL)
	{
		val = token_to_value(logflags, ce->ce_vardata);

		if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
			mask |= val;
		else
		{
			slog(LG_INFO, "%s:%d: unknown flag: %s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_vardata);
		}
	}
	for (flce = ce->ce_entries; flce; flce = flce->ce_next)
	{
		val = token_to_value(logflags, flce->ce_varname);

		if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
			mask |= val;
		else
		{
			slog(LG_INFO, "%s:%d: unknown flag: %s", flce->ce_fileptr->cf_filename, flce->ce_varlinenum, flce->ce_varname);
		}
	}
	log_master_set_mask(mask);

	return 0;
}

static int c_si_maxlogins(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.maxlogins = ce->ce_vardatanum;

	return 0;

}

static int c_si_maxusers(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.maxusers = ce->ce_vardatanum;

	return 0;

}

static int c_si_maxnicks(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.maxnicks = ce->ce_vardatanum;

	return 0;
}

static int c_si_maxchans(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.maxchans = ce->ce_vardatanum;

	return 0;
}

static int c_si_emaillimit(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.emaillimit = ce->ce_vardatanum;

	return 0;
}

static int c_si_emailtime(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.emailtime = ce->ce_vardatanum;

	return 0;
}

static int c_si_auth(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (!strcasecmp("EMAIL", ce->ce_vardata))
		me.auth = AUTH_EMAIL;

	else
		me.auth = AUTH_NONE;

	return 0;
}

static int c_si_casemapping(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (!strcasecmp("ASCII", ce->ce_vardata))
		set_match_mapping(MATCH_ASCII);

	else
		set_match_mapping(MATCH_RFC1459);

	return 0;
}

static int c_ci_vop(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.ca_vop = flags_to_bitmask(ce->ce_vardata, chanacs_flags, 0);

	return 0;
}

static int c_ci_hop(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.ca_hop = flags_to_bitmask(ce->ce_vardata, chanacs_flags, 0);

	return 0;
}

static int c_ci_aop(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.ca_aop = flags_to_bitmask(ce->ce_vardata, chanacs_flags, 0);

	return 0;
}

static int c_ci_sop(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.ca_sop = flags_to_bitmask(ce->ce_vardata, chanacs_flags, 0);

	return 0;
}

static int c_ci_trigger(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (chansvs.trigger != NULL)
		free(chansvs.trigger);
	chansvs.trigger = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ci_expire(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.expiry = (ce->ce_vardatanum * 60 * 60 * 24);

	return 0;
}

static int c_ci_maxchanacs(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.maxchanacs = ce->ce_vardatanum;

	return 0;
}

static int c_ci_maxfounders(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.maxfounders = ce->ce_vardatanum;

	return 0;
}

static int c_ci_deftemplates(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (chansvs.deftemplates != NULL)
		free(chansvs.deftemplates);
	chansvs.deftemplates = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gi_chan(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.chan = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gi_uflags(config_entry_t *ce)
{
	config_entry_t *flce;

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

static int c_gi_cflags(config_entry_t *ce)
{
	config_entry_t *flce;

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

	if (config_options.defcflags & MC_TOPICLOCK)
		config_options.defcflags |= MC_KEEPTOPIC;

	return 0;
}

static int c_gi_flood_msgs(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.flood_msgs = ce->ce_vardatanum;

	return 0;
}

static int c_gi_default_clone_limit(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.default_clone_limit = ce->ce_vardatanum;

	return 0;
}

static int c_gi_uplink_sendq_limit(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.uplink_sendq_limit = ce->ce_vardatanum;
	if (curr_uplink && curr_uplink->conn)
		sendq_set_limit(curr_uplink->conn, config_options.uplink_sendq_limit);

	return 0;
}

static int c_gi_flood_time(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.flood_time = ce->ce_vardatanum;

	return 0;
}

static int c_gi_kline_time(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.kline_time = (ce->ce_vardatanum * 60 * 60 * 24);

	return 0;
}

static int c_gi_commit_interval(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.commit_interval = (ce->ce_vardatanum * 60);

	return 0;
}

static int c_gi_expire(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	slog(LG_INFO, "warning: general::expire has been deprecated. please use nickserv::expire and chanserv::expire respectively.");

	nicksvs.expiry = (ce->ce_vardatanum * 60 * 60 * 24);
	chansvs.expiry = (ce->ce_vardatanum * 60 * 60 * 24);

	return 0;
}

static int c_ni_expire(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	nicksvs.expiry = (ce->ce_vardatanum * 60 * 60 * 24);

	return 0;
}

static int c_ni_enforce_expire(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	nicksvs.enforce_expiry = (ce->ce_vardatanum * 60 * 60 * 24);

	return 0;
}

static int c_ni_enforce_delay(config_entry_t *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	nicksvs.enforce_delay = ce->ce_vardatanum;

	return 0;
}

static int c_logfile(config_entry_t *ce)
{
	config_entry_t *flce;
	unsigned int logval = 0;

	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	for (flce = ce->ce_entries; flce; flce = flce->ce_next)
	{
		int val;

		val = token_to_value(logflags, flce->ce_varname);

		if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
			logval |= val;
		else
		{
			slog(LG_INFO, "%s:%d: unknown flag: %s", flce->ce_fileptr->cf_filename, flce->ce_varlinenum, flce->ce_varname);
		}
	}

	logfile_new(ce->ce_vardata, logval);

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
	dst->maxlogins = src->maxlogins;
	dst->maxusers = src->maxusers;
	dst->maxnicks = src->maxnicks;
	dst->maxchans = src->maxchans;
	dst->emaillimit = src->emaillimit;
	dst->emailtime = src->emailtime;
	dst->auth = src->auth;
}

static void free_cstructs(struct me *mesrc)
{
	free(mesrc->netname);
	free(mesrc->hidehostsuffix);
	free(mesrc->adminname);
	free(mesrc->adminemail);
	free(mesrc->mta);
}

bool conf_rehash(void)
{
	struct me *hold_me = scalloc(sizeof(struct me), 1);	/* and keep_me_warm; */
	char *oldsnoop;
	config_file_t *cfp;

	/* we're rehashing */
	slog(LG_INFO, "conf_rehash(): rehashing");
	runflags |= RF_REHASHING;

	cfp = config_load(config_file);
	if (cfp == NULL)
	{
		slog(LG_ERROR, "conf_rehash(): unable to load configuration file, aborting rehash");
		runflags &= ~RF_REHASHING;
		return false;
	}

	copy_me(&me, hold_me);

	oldsnoop = config_options.chan != NULL ? sstrdup(config_options.chan) :
		NULL;

	/* reset everything */
	conf_init();
	mark_all_illegal();
	log_shutdown();

	/* now reload */
	log_open();
	conf_process(cfp);
	config_free(cfp);
	hook_call_event("config_ready", NULL);

	/* now recheck */
	if (!conf_check())
	{
		slog(LG_ERROR, "conf_rehash(): conf file was malformed, aborting rehash");

		/* freeing the new conf strings */
		free_cstructs(&me);

		/* return everything to the way it was before */
		copy_me(hold_me, &me);

		/* not fully ok, oh well */
		unmark_all_illegal();

		free(hold_me);
		free(oldsnoop);

		runflags &= ~RF_REHASHING;
		return false;
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
	return true;
}

bool conf_check(void)
{
	if (!me.name)
	{
		slog(LG_ERROR, "conf_check(): no `name' set in %s", config_file);
		return false;
	}

	/* The following checks could perhaps be stricter */
	if (!strchr(me.name, '.') || strchr("!\"#$%&+,-./:@", me.name[0]) ||
			strchr(me.name, ' '))
	{
		slog(LG_ERROR, "conf_check(): bogus `name' in %s (did you specify a valid server name?)", config_file);
		return false;
	}

	if (isdigit(me.name[0]))
		slog(LG_ERROR, "conf_check(): `name' in %s starts with a digit, probably invalid (continuing anyway)", config_file);

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
		return false;
	}

	if (!me.adminname)
	{
		slog(LG_INFO, "conf_check(): no `adminname' set in %s", config_file);
		return false;
	}

	if (!me.adminemail)
	{
		slog(LG_INFO, "conf_check(): no `adminemail' set in %s", config_file);
		return false;
	}

	if (!me.mta && me.auth == AUTH_EMAIL)
	{
		slog(LG_INFO, "conf_check(): no `mta' set in %s (but `auth' is email)", config_file);
		return false;
	}

	if (!me.maxlogins)
	{
		slog(LG_INFO, "conf_check(): no `maxlogins' set in %s; " "defaulting to 5", config_file);
		me.maxlogins = 5;
	}

	if (!me.maxnicks)
	{
		if (!nicksvs.no_nick_ownership)
			slog(LG_INFO, "conf_check(): no `maxnicks' set in %s; " "defaulting to 5", config_file);
		me.maxnicks = 5;
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

	/* we know ca_all now */
	chansvs.ca_vop &= ca_all;
	chansvs.ca_hop &= ca_all;
	chansvs.ca_aop &= ca_all;
	chansvs.ca_sop &= ca_all;
	/* chansvs.ca_hop may be equal to chansvs.ca_vop to disable HOP */
	if (!chansvs.ca_vop || !chansvs.ca_hop || !chansvs.ca_aop ||
			!chansvs.ca_sop ||
			chansvs.ca_vop == chansvs.ca_aop ||
			chansvs.ca_vop == chansvs.ca_sop ||
			chansvs.ca_hop == chansvs.ca_aop ||
			chansvs.ca_hop == chansvs.ca_sop ||
			chansvs.ca_aop == chansvs.ca_sop)
	{
		slog(LG_INFO, "conf_check(): invalid xop levels in %s, using defaults", config_file);
		chansvs.ca_vop = CA_VOP_DEF & ca_all;
		chansvs.ca_hop = CA_HOP_DEF & ca_all;
		chansvs.ca_aop = CA_AOP_DEF & ca_all;
		chansvs.ca_sop = CA_SOP_DEF & ca_all;
	}

	if (config_options.flood_msgs && !config_options.flood_time)
		config_options.flood_time = 10;

	if (!config_options.default_clone_limit)
		config_options.default_clone_limit = 6;

	/* recall that commit_interval is in seconds */
	if ((!config_options.commit_interval) || (config_options.commit_interval < 60) || (config_options.commit_interval > 3600))
	{
		slog(LG_INFO, "conf_check(): invalid `commit_interval' set in %s; " "defaulting to 5 minutes", config_file);
		config_options.commit_interval = 300;
	}

	return true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
