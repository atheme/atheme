/*
 * atheme-services: A collection of minimalist IRC services   
 * conf.c: Services-specific configuration processing.
 *
 * Copyright (c) 2005-2008 Atheme Project (http://www.atheme.org)           
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
#include "conf.h"
#include "uplink.h"
#include "pmodule.h"
#include "privs.h"
#include "datastream.h"
#include "template.h"
#include <limits.h>

static int c_uplink(mowgli_config_file_entry_t *);
static int c_loadmodule(mowgli_config_file_entry_t *);
static int c_operclass(mowgli_config_file_entry_t *);
static int c_operator(mowgli_config_file_entry_t *);
static int c_language(mowgli_config_file_entry_t *);
static int c_string(mowgli_config_file_entry_t *);
static int c_logfile(mowgli_config_file_entry_t *);

static int c_si_loglevel(mowgli_config_file_entry_t *);
static int c_si_auth(mowgli_config_file_entry_t *);
static int c_si_casemapping(mowgli_config_file_entry_t *);

static int c_gi_uflags(mowgli_config_file_entry_t *);
static int c_gi_cflags(mowgli_config_file_entry_t *);
static int c_gi_exempts(mowgli_config_file_entry_t *);
static int c_gi_immune_level(mowgli_config_file_entry_t *);

/* *INDENT-OFF* */

static struct Token uflags[] = {
  { "HOLD",      MU_HOLD         },
  { "NEVEROP",   MU_NEVEROP      },
  { "NOOP",      MU_NOOP         },
  { "HIDEMAIL",  MU_HIDEMAIL     },
  { "NOMEMO",    MU_NOMEMO       },
  { "EMAILMEMOS",MU_EMAILMEMOS   },
  { "ENFORCE",   MU_ENFORCE      },
  { "PRIVMSG",   MU_USE_PRIVMSG  },
  { "PRIVATE",   MU_PRIVATE      },
  { "QUIETCHG",  MU_QUIETCHG     },
  { "NEVERGROUP", MU_NEVERGROUP  },
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
  { "GUARD",       MC_GUARD       },
  { "NOSYNC",	   MC_NOSYNC	  },
  { "PRIVATE",     MC_PRIVATE     },
  { "LIMITFLAGS",  MC_LIMITFLAGS  },
  { "ANTIFLOOD",   MC_ANTIFLOOD   },
  { "NONE",        0              },
  { NULL, 0 }
};

static struct Token operflags[] = {
  { "IMMUNE",      UF_IMMUNE      },
  { "ADMIN",       UF_ADMIN       },
  { "IRCOP",       UF_IRCOP       },
  { NULL, 0 }
};

static struct Token logflags[] = {
  { "DEBUG",       LG_ALL                                                                                     },
  { "TRACE",       LG_INFO | LG_ERROR | LG_CMD_ALL | LG_NETWORK | LG_WALLOPS | LG_REGISTER                    },
  { "MISC",        LG_INFO | LG_ERROR | LG_CMD_ADMIN | LG_CMD_REGISTER | LG_CMD_SET | LG_NETWORK | LG_WALLOPS | LG_REGISTER },
  { "NOTICE",      LG_INFO | LG_ERROR | LG_CMD_ADMIN | LG_CMD_REGISTER | LG_NETWORK | LG_REGISTER             },
  { "ALL",         LG_ALL                                                                                     },
  { "INFO",        LG_INFO                                                                                    },
  { "VERBOSE",	   LG_VERBOSE										      },
  { "ERROR",       LG_ERROR                                                                                   },
  { "COMMANDS",    LG_CMD_ALL                                                                                 },
  { "ADMIN",       LG_CMD_ADMIN                                                                               },
  { "REGISTER",    LG_CMD_REGISTER | LG_REGISTER                                                              },
  { "SET",         LG_CMD_SET                                                                                 },
  { "REQUEST",     LG_CMD_REQUEST									      },
  { "NETWORK",     LG_NETWORK                                                                                 },
  { "WALLOPS",     LG_WALLOPS                                                                                 },
  { "RAWDATA",     LG_RAWDATA                                                                                 },
  { "DENYCMD",     LG_DENYCMD                                                                                 },
  { NULL,          0                                                                                          },
};

mowgli_list_t conf_si_table;
mowgli_list_t conf_gi_table;
mowgli_list_t conf_la_table;

/* *INDENT-ON* */

const char *get_conf_opts(void)
{
	static char opts[53];

	snprintf(opts, sizeof opts, "%s%s%s%s%s%s%s%s%s%s%s%s%s",
			match_mapping ? "A" : "",
			auth_module_loaded ? "a" : "",
			crypto_module_loaded ? "c" : "",
			log_debug_enabled() ? "d" : "",
			me.auth ? "e" : "",
			config_options.flood_msgs ? "F" : "",
			config_options.leave_chans ? "l" : "", 
			config_options.join_chans ? "j" : "", 
			chansvs.changets ? "t" : "",
			!match_mapping ? "R" : "",
			config_options.raw ? "r" : "",
			runflags & RF_LIVE ? "n" : "",
			IS_TAINTED ? "T" : "");
	return opts;
}

bool conf_parse(const char *file)
{
	mowgli_config_file_t *cfp;

	cfp = mowgli_config_file_load(file);
	if (cfp == NULL)
		return false;

	conf_process(cfp);
	mowgli_config_file_free(cfp);

	if (!pmodule_loaded)
	{
		slog(LG_ERROR, "No protocol module loaded, aborting");
		return false;
	}
	update_chanacs_flags();
	if (!conf_check())
		return false;

	TAINT_ON(config_options.raw, "raw can be used to cause network desyncs and therefore is unsupported.");

	hook_call_config_ready();
	return true;
}

void conf_init(void)
{
	hook_call_config_purge();

	config_options.defuflags = config_options.defcflags = 0x00000000;
	config_options.immune_level = UF_IMMUNE;

	me.auth = AUTH_NONE;

	clear_global_template_flags();
	set_global_template_flags("VOP", CA_VOP_DEF);
	set_global_template_flags("HOP", CA_HOP_DEF);
	set_global_template_flags("AOP", CA_AOP_DEF);
	set_global_template_flags("SOP", CA_SOP_DEF);

	if (!(runflags & RF_REHASHING))
	{
		set_match_mapping(MATCH_RFC1459);	/* default to RFC compliancy */
	}
}

void init_newconf(void)
{
	/* First we set up the blocks. */
	add_subblock_top_conf("SERVERINFO", &conf_si_table);
	add_top_conf("UPLINK", c_uplink);
	add_subblock_top_conf("GENERAL", &conf_gi_table);
	add_top_conf("LOADMODULE", c_loadmodule);
	add_top_conf("OPERCLASS", c_operclass);
	add_top_conf("OPERATOR", c_operator);
	add_top_conf("LANGUAGE", c_language);
	add_top_conf("STRING", c_string);
	add_top_conf("LOGFILE", c_logfile);

	/* serverinfo{} block */
	add_dupstr_conf_item("NAME", &conf_si_table, CONF_NO_REHASH, &me.name, NULL);
	add_dupstr_conf_item("DESC", &conf_si_table, CONF_NO_REHASH, &me.desc, NULL);
	add_dupstr_conf_item("NUMERIC", &conf_si_table, CONF_NO_REHASH, &me.numeric, NULL);
	add_dupstr_conf_item("VHOST", &conf_si_table, CONF_NO_REHASH, &me.vhost, NULL);
	add_duration_conf_item("RECONTIME", &conf_si_table, 0, &me.recontime, "s", 10);
	add_duration_conf_item("RESTARTTIME", &conf_si_table, 0, &me.restarttime, "s", 0);
	add_dupstr_conf_item("NETNAME", &conf_si_table, 0, &me.netname, NULL);
	add_dupstr_conf_item("HIDEHOSTSUFFIX", &conf_si_table, 0, &me.hidehostsuffix, NULL);
	add_dupstr_conf_item("ADMINNAME", &conf_si_table, 0, &me.adminname, NULL);
	add_dupstr_conf_item("ADMINEMAIL", &conf_si_table, 0, &me.adminemail, NULL);
	add_dupstr_conf_item("REGISTEREMAIL", &conf_si_table, 0, &me.register_email, NULL);
	add_dupstr_conf_item("MTA", &conf_si_table, 0, &me.mta, NULL);
	add_conf_item("LOGLEVEL", &conf_si_table, c_si_loglevel);
	add_uint_conf_item("MAXLOGINS", &conf_si_table, 0, &me.maxlogins, 3, INT_MAX, 5);
	add_uint_conf_item("MAXUSERS", &conf_si_table, 0, &me.maxusers, 0, INT_MAX, 0);
	add_uint_conf_item("EMAILLIMIT", &conf_si_table, 0, &me.emaillimit, 1, INT_MAX, 10);
	add_duration_conf_item("EMAILTIME", &conf_si_table, 0, &me.emailtime, "s", 300);
	add_conf_item("AUTH", &conf_si_table, c_si_auth);
	add_uint_conf_item("MDLIMIT", &conf_si_table, 0, &me.mdlimit, 0, INT_MAX, 30);
	add_conf_item("CASEMAPPING", &conf_si_table, c_si_casemapping);
	add_bool_conf_item("HIDDEN", &conf_si_table, 0, &me.hidden, false);

	/* general{} block */
	add_dupstr_conf_item("HELPCHAN", &conf_gi_table, 0, &config_options.helpchan, NULL);
	add_dupstr_conf_item("HELPURL", &conf_gi_table, 0, &config_options.helpurl, NULL);
	add_bool_conf_item("PERMISSIVE_MODE", &conf_gi_table, 0, &permissive_mode, false);
	add_bool_conf_item("VERBOSE_WALLOPS", &conf_gi_table, 0, &config_options.verbose_wallops, false);
	add_bool_conf_item("ALLOW_TAINT", &conf_gi_table, 0, &config_options.allow_taint, false);
	add_bool_conf_item("SILENT", &conf_gi_table, 0, &config_options.silent, false);
	add_bool_conf_item("JOIN_CHANS", &conf_gi_table, 0, &config_options.join_chans, false);
	add_bool_conf_item("LEAVE_CHANS", &conf_gi_table, 0, &config_options.leave_chans, false);
	add_bool_conf_item("KLINE_WITH_IDENT", &conf_gi_table, 0, &config_options.kline_with_ident, false);
	add_bool_conf_item("KLINE_VERIFIED_IDENT", &conf_gi_table, 0, &config_options.kline_verified_ident, false);
	add_conf_item("UFLAGS", &conf_gi_table, c_gi_uflags);
	add_conf_item("CFLAGS", &conf_gi_table, c_gi_cflags);
	add_bool_conf_item("RAW", &conf_gi_table, 0, &config_options.raw, false);
	add_bool_conf_item("SECURE", &conf_gi_table, 0, &config_options.secure, false);
	add_uint_conf_item("FLOOD_MSGS", &conf_gi_table, 0, &config_options.flood_msgs, 0, INT_MAX, 0);
	add_duration_conf_item("FLOOD_TIME", &conf_gi_table, 0, &config_options.flood_time, "s", 10);
	add_uint_conf_item("RATELIMIT_USES", &conf_gi_table, 0, &config_options.ratelimit_uses, 0, INT_MAX, 0);
	add_duration_conf_item("RATELIMIT_PERIOD", &conf_gi_table, 0, &config_options.ratelimit_period, "s", 0);
	add_duration_conf_item("KLINE_TIME", &conf_gi_table, 0, &config_options.kline_time, "d", 0);
	add_duration_conf_item("CLONE_TIME", &conf_gi_table, 0, &config_options.clone_time, "m", 0);
	add_duration_conf_item("COMMIT_INTERVAL", &conf_gi_table, 0, &config_options.commit_interval, "m", 300);
	/* XXX: These options should probably move into operserv/clones eventually */
	add_uint_conf_item("DEFAULT_CLONE_WARN", &conf_gi_table, 0, &config_options.default_clone_warn, 1, INT_MAX, 5);
	add_uint_conf_item("DEFAULT_CLONE_ALLOWED", &conf_gi_table, 0, &config_options.default_clone_allowed, 1, INT_MAX, 5);
	add_bool_conf_item("CLONE_IDENTIFIED_INCREASE_LIMIT", &conf_gi_table, 0, &config_options.clone_increase, false);

	add_uint_conf_item("UPLINK_SENDQ_LIMIT", &conf_gi_table, 0, &config_options.uplink_sendq_limit, 10240, INT_MAX, 1048576);
	add_dupstr_conf_item("LANGUAGE", &conf_gi_table, 0, &config_options.language, "en");
	add_conf_item("EXEMPTS", &conf_gi_table, c_gi_exempts);
	add_conf_item("IMMUNE_LEVEL", &conf_gi_table, c_gi_immune_level);

	/* language:: stuff */
	add_dupstr_conf_item("NAME", &conf_la_table, 0, &me.language_name, NULL);
	add_dupstr_conf_item("TRANSLATOR", &conf_la_table, 0, &me.language_translator, NULL);
}

static int c_loadmodule(mowgli_config_file_entry_t *ce)
{
	char pathbuf[4096];
	char *name;

	if (!cold_start)
		return 0;

	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	name = ce->vardata;

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

static int c_uplink(mowgli_config_file_entry_t *ce)
{
	char *name;
	char *host = NULL, *vhost = NULL, *send_password = NULL, *receive_password = NULL;
	unsigned int port = 0;

	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	if (me.name != NULL && !irccasecmp(ce->vardata, me.name))
		conf_report_warning(ce, "uplink's server name %s should not be the same as our server name, continuing anyway", ce->vardata);
	else if (!strchr(ce->vardata, '.'))
		conf_report_warning(ce, "uplink's server name %s is invalid, continuing anyway", ce->vardata);
	else if (isdigit(ce->vardata[0]))
		conf_report_warning(ce, "uplink's server name %s starts with a digit, probably invalid (continuing anyway)", ce->vardata);

	name = ce->vardata;

	MOWGLI_ITER_FOREACH(ce, ce->entries)
	{
		if (!strcasecmp("HOST", ce->varname))
		{
			if (ce->vardata == NULL)
			{
				conf_report_warning(ce, "no parameter for configuration option");
				return 0;
			}

			host = ce->vardata;
		}
		else if (!strcasecmp("VHOST", ce->varname))
		{
			if (ce->vardata == NULL)
			{
				conf_report_warning(ce, "no parameter for configuration option");
				return 0;
			}

			vhost = ce->vardata;
		}
		else if (!strcasecmp("PASSWORD", ce->varname))
		{
			if (ce->vardata == NULL)
			{
				conf_report_warning(ce, "no parameter for configuration option");
				return 0;
			}

			send_password = ce->vardata;
			receive_password = ce->vardata;
		}
		else if (!strcasecmp("SEND_PASSWORD", ce->varname))
		{
			if (ce->vardata == NULL)
			{
				conf_report_warning(ce, "no parameter for configuration option");
				return 0;
			}

			send_password = ce->vardata;
		}
		else if (!strcasecmp("RECEIVE_PASSWORD", ce->varname))
		{
			if (ce->vardata == NULL)
			{
				conf_report_warning(ce, "no parameter for configuration option");
				return 0;
			}

			receive_password = ce->vardata;
		}
		else if (!strcasecmp("PORT", ce->varname))
		{
			(void)process_uint_configentry(ce, &port, 1, 65535);
		}
		else
		{
			conf_report_warning(ce, "Invalid configuration option");
			continue;
		}
	}

	if (strchr(send_password, ' '))
		conf_report_warning(ce, "send_password for uplink %s is invalid (has spaces); continuing anyway", name);
	if (strchr(receive_password, ' '))
		conf_report_warning(ce, "receive_password for uplink %s is invalid (has spaces); continuing anyway", name);

	uplink_add(name, host, send_password, receive_password, vhost, port);
	return 0;
}

static int c_operclass(mowgli_config_file_entry_t *ce)
{
	char *name;
	char *privs = NULL, *newprivs;
	int flags = 0;

	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	name = ce->vardata;

	MOWGLI_ITER_FOREACH(ce, ce->entries)
	{
		if (!strcasecmp("PRIVS", ce->varname))
		{
			if (ce->vardata == NULL && ce->entries == NULL)
				conf_report_warning(ce, "no parameter for configuration option");

			if (ce->vardata != NULL)
			{
				if (privs == NULL)
					privs = sstrdup(ce->vardata);
				else
				{
					newprivs = smalloc(strlen(privs) + 1 + strlen(ce->vardata) + 1);
					strcpy(newprivs, privs);
					strcat(newprivs, " ");
					strcat(newprivs, ce->vardata);
					free(privs);
					privs = newprivs;
				}
			}
			else
			{
				mowgli_config_file_entry_t *conf_p;
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
				MOWGLI_ITER_FOREACH(conf_p, ce->entries)
				{
					if (privs == NULL)
						privs = sstrdup(conf_p->varname);
					else
					{
						newprivs = smalloc(strlen(privs) + 1 + strlen(conf_p->varname) + 1);
						strcpy(newprivs, privs);
						strcat(newprivs, " ");
						strcat(newprivs, conf_p->varname);
						free(privs);
						privs = newprivs;
					}
				}
			}
		}
		else if (!strcasecmp("EXTENDS", ce->varname))
		{
			if (ce->vardata == NULL)
			{
				conf_report_warning(ce, "no parameter for configuration option");
				continue;
			}
			operclass_t *parent = operclass_find(ce->vardata);
			if (parent == NULL)
			{
				conf_report_warning(ce, "nonexistent extends operclass %s for operclass %s", ce->vardata, name);
				continue;
			}

			if (privs == NULL)
				privs = sstrdup(parent->privs);
			else
			{
				newprivs = smalloc(strlen(privs) + 1 + strlen(parent->privs) + 1);
				strcpy(newprivs, privs);
				strcat(newprivs, " ");
				strcat(newprivs, parent->privs);
				free(privs);
				privs = newprivs;
			}
		}
		else if (!strcasecmp("NEEDOPER", ce->varname))
			flags |= OPERCLASS_NEEDOPER;
		else
		{
			conf_report_warning(ce, "Invalid configuration option");
			continue;
		}
	}

	operclass_add(name, privs ? privs : "", flags);
	free(privs);
	return 0;
}

static int c_operator(mowgli_config_file_entry_t *ce)
{
	char *name;
	char *password = NULL;
	operclass_t *operclass = NULL;
	mowgli_config_file_entry_t *topce;

	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	topce = ce;
	name = ce->vardata;

	MOWGLI_ITER_FOREACH(ce, ce->entries)
	{
		if (!strcasecmp("OPERCLASS", ce->varname))
		{
			if (ce->vardata == NULL)
			{
				conf_report_warning(ce, "no parameter for configuration option");
				return 0;
			}

			operclass = operclass_find(ce->vardata);
			if (operclass == NULL)
				conf_report_warning(ce, "invalid operclass %s for operator %s",
						ce->vardata, name);
		}
		else if (!strcasecmp("PASSWORD", ce->varname))
		{
			if (ce->vardata == NULL)
			{
				conf_report_warning(ce, "no parameter for configuration option");
				return 0;
			}

			password = ce->vardata;
		}
		else
		{
			conf_report_warning(ce, "Invalid configuration option");
			continue;
		}
	}

	if (operclass != NULL)
		soper_add(name, operclass->name, SOPER_CONF, password);
	else
		conf_report_warning(topce, "skipping operator %s because of bad/missing parameters",
						name);
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
static int c_language(mowgli_config_file_entry_t *ce)
{
	if (ce->entries)
	{
		subblock_handler(ce, &conf_la_table);
		return 0;
	}
	else
		config_options.languagefile = sstrdup(ce->vardata);

	return 0;
}

static int c_string(mowgli_config_file_entry_t *ce)
{
	char *name, *trans = NULL;
	mowgli_config_file_entry_t *topce;

	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	topce = ce;
	name = ce->vardata;

	MOWGLI_ITER_FOREACH(ce, ce->entries)
	{
		if (!strcasecmp("TRANSLATION", ce->varname))
		{
			if (ce->vardata == NULL)
			{
				conf_report_warning(ce, "no parameter for configuration option");
				return 0;
			}

			trans = ce->vardata;
		}
		else
		{
			conf_report_warning(ce, "Invalid configuration option");
			continue;
		}
	}

	if (trans != NULL)
		translation_create(name, trans);
	else
		conf_report_warning(topce, "missing translation for string");
	return 0;
}

static int c_si_loglevel(mowgli_config_file_entry_t *ce)
{
	mowgli_config_file_entry_t *flce;
	int val;
	int mask = 0;

	if (ce->vardata != NULL)
	{
		val = token_to_value(logflags, ce->vardata);

		if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
			mask |= val;
		else
			conf_report_warning(ce, "unknown flag: %s", ce->vardata);
	}

	MOWGLI_ITER_FOREACH(flce, ce->entries)
	{
		val = token_to_value(logflags, flce->varname);

		if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
			mask |= val;
		else
			conf_report_warning(flce, "unknown flag: %s", flce->varname);
	}

	log_master_set_mask(mask);

	return 0;
}

static int c_si_auth(mowgli_config_file_entry_t *ce)
{
	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	if (!strcasecmp("EMAIL", ce->vardata))
		me.auth = AUTH_EMAIL;

	else
		me.auth = AUTH_NONE;

	return 0;
}

static int c_si_casemapping(mowgli_config_file_entry_t *ce)
{
	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	if (!strcasecmp("ASCII", ce->vardata))
		set_match_mapping(MATCH_ASCII);

	else
		set_match_mapping(MATCH_RFC1459);

	return 0;
}

static int c_gi_immune_level(mowgli_config_file_entry_t *ce)
{
	int val;

	val = token_to_value(operflags, ce->vardata);

	if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
		config_options.immune_level |= val;
	else
		conf_report_warning(ce, "unknown flag: %s", ce->vardata);

	return 0;
}

static int c_gi_uflags(mowgli_config_file_entry_t *ce)
{
	mowgli_config_file_entry_t *flce;

	MOWGLI_ITER_FOREACH(flce, ce->entries)
	{
		int val;

		val = token_to_value(uflags, flce->varname);

		if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
			config_options.defuflags |= val;

		else
			conf_report_warning(flce, "unknown flag: %s", flce->varname);
	}

	return 0;
}

static int c_gi_cflags(mowgli_config_file_entry_t *ce)
{
	mowgli_config_file_entry_t *flce;

	MOWGLI_ITER_FOREACH(flce, ce->entries)
	{
		int val;

		val = token_to_value(cflags, flce->varname);

		if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
			config_options.defcflags |= val;

		else
			conf_report_warning(flce, "unknown flag: %s", flce->varname);
	}

	if (config_options.defcflags & MC_TOPICLOCK)
		config_options.defcflags |= MC_KEEPTOPIC;

	return 0;
}

static int c_gi_exempts(mowgli_config_file_entry_t *ce)
{
	mowgli_config_file_entry_t *subce;
	mowgli_node_t *n, *tn;

	if (!ce->entries)
		return 0;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, config_options.exempts.head)
	{
		free(n->data);
		mowgli_node_delete(n, &config_options.exempts);
		mowgli_node_free(n);
	}

	MOWGLI_ITER_FOREACH(subce, ce->entries)
	{
		if (subce->entries != NULL)
		{
			conf_report_warning(ce, "Invalid exempt entry");
			continue;
		}
		mowgli_node_add(sstrdup(subce->varname), mowgli_node_create(), &config_options.exempts);
	}

	return 0;
}


static int c_logfile(mowgli_config_file_entry_t *ce)
{
	mowgli_config_file_entry_t *flce;
	unsigned int logval = 0;

	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	MOWGLI_ITER_FOREACH(flce, ce->entries)
	{
		int val;

		val = token_to_value(logflags, flce->varname);

		if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
			logval |= val;
		else
		{
			conf_report_warning(flce, "unknown flag: %s", flce->varname);
		}
	}

	logfile_new(ce->vardata, logval);

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
	dst->register_email = sstrdup(src->register_email);
	dst->mta = src->mta ? sstrdup(src->mta) : NULL;
	dst->maxlogins = src->maxlogins;
	dst->maxusers = src->maxusers;
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
	free(mesrc->register_email);
	free(mesrc->mta);
}

bool conf_rehash(void)
{
	struct me *hold_me = scalloc(sizeof(struct me), 1);	/* and keep_me_warm; */
	mowgli_config_file_t *cfp;

	/* we're rehashing */
	slog(LG_INFO, "conf_rehash(): rehashing");
	runflags |= RF_REHASHING;

	cfp = mowgli_config_file_load(config_file);
	if (cfp == NULL)
	{
		slog(LG_ERROR, "conf_rehash(): unable to load configuration file, aborting rehash");
		runflags &= ~RF_REHASHING;
		return false;
	}

	copy_me(&me, hold_me);

	/* reset everything */
	conf_init();
	mark_all_illegal();
	log_shutdown();

	/* now reload */
	log_open();
	conf_process(cfp);
	mowgli_config_file_free(cfp);

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

		free_cstructs(hold_me);
		free(hold_me);

		runflags &= ~RF_REHASHING;
		return false;
	}

	hook_call_config_ready();

	if (curr_uplink && curr_uplink->conn)
		sendq_set_limit(curr_uplink->conn, config_options.uplink_sendq_limit);

	remove_illegals();

	free_cstructs(hold_me);
	free(hold_me);

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

	if (!me.register_email)
	{
		char buf[BUFSIZE];

		mowgli_strlcpy(buf, "noreply.", sizeof buf);
		mowgli_strlcat(buf, me.adminemail, sizeof buf);

		me.register_email = sstrdup(buf);
		slog(LG_INFO, "conf_check(): no `registeremail' set in %s, using `%s' based on `adminemail'", config_file, me.register_email);
	}

	if (!me.mta && me.auth == AUTH_EMAIL)
	{
		slog(LG_INFO, "conf_check(): no `mta' set in %s (but `auth' is email)", config_file);
		return false;
	}

	if (me.auth != 0 && me.auth != 1)
	{
		slog(LG_INFO, "conf_check(): no `auth' set in %s; defaulting to NONE", config_file);
		me.auth = AUTH_NONE;
	}

	if (chansvs.hide_xop)
	{
		mowgli_patricia_delete(global_template_dict,"VOP");
		mowgli_patricia_delete(global_template_dict,"HOP");
		mowgli_patricia_delete(global_template_dict,"AOP");
		mowgli_patricia_delete(global_template_dict,"SOP");
	}
	else
	{
		/* we know ca_all now, so remove invalid flags */
		fix_global_template_flags();
	}

	if (config_options.commit_interval < 60 || config_options.commit_interval > 3600)
	{
		slog(LG_INFO, "conf_check(): invalid `commit_interval' set in %s; defaulting to 5 minutes", config_file);
		config_options.commit_interval = 300;
	}

	return true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
