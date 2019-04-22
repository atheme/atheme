/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2011 William Pitcock, et al.
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains routines to handle the OperServ SET command.
 */

#include <atheme.h>

static mowgli_patricia_t *os_set_cmdtree = NULL;

static void
os_help_set(struct sourceinfo *const restrict si, const char *const restrict subcmd)
{
	if (subcmd)
	{
		(void) help_display_as_subcmd(si, si->service, "SET", subcmd, os_set_cmdtree);
		return;
	}

	(void) help_display_prefix(si, si->service);
	(void) command_success_nodata(si, _("Help for \2SET\2:"));
	(void) help_display_newline(si);

	(void) command_success_nodata(si, _("SET allows you to set various control flags for\n"
	                                    "services that changes the way certain operations\n"
	                                    "are performed by them. Note that all settings\n"
	                                    "will be re-set to the values in the configuration\n"
	                                    "file when services is rehashed or restarted."));

	(void) help_display_newline(si);
	(void) command_help(si, os_set_cmdtree);
	(void) help_display_moreinfo(si, si->service, "SET");
	(void) help_display_suffix(si);
}

static void
os_cmd_set(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET <setting> <parameters>"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, os_set_cmdtree, "SET");
}

static void
os_cmd_set_recontime(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET RECONTIME");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET RECONTIME <seconds>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value) || ! value)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET RECONTIME");
		(void) command_fail(si, fault_badparams, _("Syntax: SET RECONTIME <seconds>"));
		return;
	}

	me.recontime = value;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2 seconds."), "RECONTIME", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:RECONTIME: \2%u\2", value);
}

static void
os_cmd_set_maxlogins(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET MAXLOGINS");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET MAXLOGINS <value>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value) || value < 3)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET MAXLOGINS");
		(void) command_fail(si, fault_badparams, _("Syntax: SET MAXLOGINS <value>"));
		return;
	}

	me.maxlogins = value;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2."), "MAXLOGINS", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:MAXLOGINS: \2%u\2", value);
}

static void
os_cmd_set_maxusers(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET MAXUSERS");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET MAXUSERS <value>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value) || ! value)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET MAXUSERS");
		(void) command_fail(si, fault_badparams, _("Syntax: SET MAXUSERS <value>"));
		return;
	}

	me.maxusers = value;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2."), "MAXUSERS", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:MAXUSERS: \2%u\2", value);
}

static void
os_cmd_set_maxnicks(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET MAXNICKS");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET MAXNICKS <value>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value) || ! value)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET MAXNICKS");
		(void) command_fail(si, fault_badparams, _("Syntax: SET MAXNICKS <value>"));
		return;
	}

	nicksvs.maxnicks = value;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2."), "MAXNICKS", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:MAXNICKS: \2%u\2", value);
}

static void
os_cmd_set_maxchans(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET MAXCHANS");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET MAXCHANS <value>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value) || ! value)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET MAXCHANS");
		(void) command_fail(si, fault_badparams, _("Syntax: SET MAXCHANS <value>"));
		return;
	}

	chansvs.maxchans = value;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2."), "MAXCHANS", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:MAXCHANS: \2%u\2", value);
}

static void
os_cmd_set_mdlimit(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET MDLIMIT");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET MDLIMIT <value>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value) || ! value)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET MDLIMIT");
		(void) command_fail(si, fault_badparams, _("Syntax: SET MDLIMIT <value>"));
		return;
	}

	me.mdlimit = value;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2."), "MDLIMIT", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:MDLIMIT: \2%u\2", value);
}

static void
os_cmd_set_klinetime(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET KLINETIME");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET KLINETIME <days>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value) || ! value)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET KLINETIME");
		(void) command_fail(si, fault_badparams, _("Syntax: SET KLINETIME <days>"));
		return;
	}

	config_options.kline_time = value * SECONDS_PER_DAY;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2 days."), "KLINETIME", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:KLINETIME: \2%u\2", value);
}

static void
os_cmd_set_commitinterval(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET COMMITINTERVAL");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET COMMITINTERVAL <minutes>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value) || ! value)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET COMMITINTERVAL");
		(void) command_fail(si, fault_badparams, _("Syntax: SET COMMITINTERVAL <minutes>"));
		return;
	}

	config_options.commit_interval = value * SECONDS_PER_MINUTE;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2 minutes."), "COMMITINTERVAL", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:COMMITINTERVAL: \2%u\2", value);
}

static void
os_cmd_set_chanexpire(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET CHANEXPIRE");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET CHANEXPIRE <days>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value) || ! value)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET CHANEXPIRE");
		(void) command_fail(si, fault_badparams, _("Syntax: SET CHANEXPIRE <days>"));
		return;
	}

	chansvs.expiry = value * SECONDS_PER_DAY;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2 days."), "CHANEXPIRE", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:CHANEXPIRE: \2%u\2", value);
}

static void
os_cmd_set_maxchanacs(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET MAXCHANACS");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET MAXCHANACS <value>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value) || ! value)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET MAXCHANACS");
		(void) command_fail(si, fault_badparams, _("Syntax: SET MAXCHANACS <value>"));
		return;
	}

	chansvs.maxchanacs = value;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2."), "MAXCHANACS", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:MAXCHANACS: \2%u\2", value);
}

static void
os_cmd_set_maxfounders(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET MAXFOUNDERS");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET MAXFOUNDERS <value>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	/* Yes, I know how arbitrary the high value is, this is what it is in confprocess.c
	 * (I rounded it down though) -- JD
	 */
	if (! string_to_uint(param, &value) || ! value || value > 41)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET MAXFOUNDERS");
		(void) command_fail(si, fault_badparams, _("Syntax: SET MAXFOUNDERS <value>"));
		return;
	}

	chansvs.maxfounders = value;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2."), "MAXFOUNDERS", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:MAXFOUNDERS: \2%u\2", value);
}

static void
os_cmd_set_akicktime(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET AKICKTIME");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET AKICKTIME <minutes>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value) || ! value)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET AKICKTIME");
		(void) command_fail(si, fault_badparams, _("Syntax: SET AKICKTIME <minutes>"));
		return;
	}

	chansvs.akick_time = value * SECONDS_PER_MINUTE;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2 minutes."), "AKICKTIME", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:AKICKTIME: \2%u\2", value);
}

static void
os_cmd_set_spam(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET SPAM");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET SPAM <TRUE|FALSE>"));
		return;
	}

	const char *const param = parv[0];

	if (!strcasecmp("TRUE", param) || !strcasecmp("ON", param))
	{
		if (nicksvs.spam)
		{
			(void) command_fail(si, fault_nochange, _("The SPAM directive is already set."));
			return;
		}

		nicksvs.spam = true;

		(void) command_success_nodata(si, _("The SPAM directive has been successfully set."));
		(void) logcommand(si, CMDLOG_ADMIN, "SET:SPAM:TRUE");
	}
	else if (!strcasecmp("FALSE", param) || !strcasecmp("OFF", param))
	{
		if (!nicksvs.spam)
		{
			(void) command_fail(si, fault_nochange, _("The SPAM directive is already unset."));
			return;
		}

		nicksvs.spam = false;

		(void) command_success_nodata(si, _("The SPAM directive has been successfully unset."));
		(void) logcommand(si, CMDLOG_ADMIN, "SET:SPAM:FALSE");
	}
	else
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET SPAM");
		(void) command_fail(si, fault_badparams, _("Syntax: SET SPAM <TRUE|FALSE>"));
	}
}

static void
os_cmd_set_nickexpire(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET NICKEXPIRE");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET NICKEXPIRE <days>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value) || ! value)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET NICKEXPIRE");
		(void) command_fail(si, fault_badparams, _("Syntax: SET NICKEXPIRE <days>"));
		return;
	}

	nicksvs.expiry = value * SECONDS_PER_DAY;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2 days."), "NICKEXPIRE", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:NICKEXPIRE: \2%u\2", value);
}

static void
os_cmd_set_enforceprefix(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET ENFORCEPREFIX");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET ENFORCEPREFIX <prefix>"));
		return;
	}

	const char *const param = parv[0];

	if (! *param || ! is_valid_nick(param))
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET ENFORCEPREFIX");
		(void) command_fail(si, fault_badparams, _("Syntax: SET ENFORCEPREFIX <prefix>"));
		return;
	}

	(void) sfree(nicksvs.enforce_prefix);

	nicksvs.enforce_prefix = sstrdup(param);

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%s\2."), "ENFORCEPREFIX", param);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:ENFORCEPREFIX: \2%s\2", param);
}

static struct command os_set = {
	.name           = "SET",
	.desc           = N_("Sets various control flags."),
	.access         = PRIV_ADMIN,
	.maxparc        = 2,
	.cmd            = &os_cmd_set,
	.help           = { .func = &os_help_set },
};

static struct command os_set_recontime = {
	.name           = "RECONTIME",
	.desc           = N_("Changes IRCd reconnect delay."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_recontime,
	.help           = { .path = "oservice/set_recontime" },
};

static struct command os_set_maxlogins = {
	.name           = "MAXLOGINS",
	.desc           = N_("Changes the maximum number of users that may be logged in to one account."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_maxlogins,
	.help           = { .path = "oservice/set_maxlogins" },
};

static struct command os_set_maxnicks = {
	.name           = "MAXNICKS",
	.desc           = N_("Changes the maximum number of nicks that one account may own."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_maxnicks,
	.help           = { .path = "oservice/set_maxnicks" },
};

static struct command os_set_maxusers = {
	.name           = "MAXUSERS",
	.desc           = N_("Changes the maximum number of accounts that one email address may have registered."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_maxusers,
	.help           = { .path = "oservice/set_maxusers" },
};

static struct command os_set_maxchans = {
	.name           = "MAXCHANS",
	.desc           = N_("Changes the maximum number of channels one account may own."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_maxchans,
	.help           = { .path = "oservice/set_maxchans" },
};

static struct command os_set_mdlimit = {
	.name           = "MDLIMIT",
	.desc           = N_("Sets the maximum amount of metadata one channel or account may have."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_mdlimit,
	.help           = { .path = "oservice/set_mdlimit" },
};

static struct command os_set_klinetime = {
	.name           = "KLINETIME",
	.desc           = N_("Sets the default KLINE/AKILL time."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_klinetime,
	.help           = { .path = "oservice/set_klinetime" },
};

static struct command os_set_commitinterval = {
	.name           = "COMMITINTERVAL",
	.desc           = N_("Changes how often the database is written to disk."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_commitinterval,
	.help           = { .path = "oservice/set_commitinterval" },
};

static struct command os_set_chanexpire = {
	.name           = "CHANEXPIRE",
	.desc           = N_("Sets when unused channels expire."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_chanexpire,
	.help           = { .path = "oservice/set_chanexpire" },
};

static struct command os_set_maxchanacs = {
	.name           = "MAXCHANACS",
	.desc           = N_("Changes the maximum number of channel access list entries per channel."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_maxchanacs,
	.help           = { .path = "oservice/set_maxchanacs" },
};

static struct command os_set_maxfounders = {
	.name           = "MAXFOUNDERS",
	.desc           = N_("Sets the maximum number of founders per channel."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_maxfounders,
	.help           = { .path = "oservice/set_maxfounders" },
};

static struct command os_set_akicktime = {
	.name           = "AKICKTIME",
	.desc           = N_("Sets the default AKICK time."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_akicktime,
	.help           = { .path = "oservice/set_akicktime" },
};

static struct command os_set_spam = {
	.name           = "SPAM",
	.desc           = N_("Changes whether service spams unregistered users on connect."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_spam,
	.help           = { .path = "oservice/set_spam" },
};

static struct command os_set_nickexpire = {
	.name           = "NICKEXPIRE",
	.desc           = N_("Sets when unused nicks and accounts expire."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_nickexpire,
	.help           = { .path = "oservice/set_nickexpire" },
};

static struct command os_set_enforceprefix = {
	.name           = "ENFORCEPREFIX",
	.desc           = N_("Changes the prefix to use when changing the user's nick on enforcement."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_enforceprefix,
	.help           = { .path = "oservice/set_enforceprefix" },
};

static void
mod_init(struct module *const restrict m)
{
	if (! (os_set_cmdtree = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&os_set_recontime, os_set_cmdtree);
	(void) command_add(&os_set_maxlogins, os_set_cmdtree);
	(void) command_add(&os_set_maxnicks, os_set_cmdtree);
	(void) command_add(&os_set_maxusers, os_set_cmdtree);
	(void) command_add(&os_set_maxchans, os_set_cmdtree);
	(void) command_add(&os_set_mdlimit, os_set_cmdtree);
	(void) command_add(&os_set_klinetime, os_set_cmdtree);
	(void) command_add(&os_set_commitinterval, os_set_cmdtree);
	(void) command_add(&os_set_chanexpire, os_set_cmdtree);
	(void) command_add(&os_set_maxchanacs, os_set_cmdtree);
	(void) command_add(&os_set_maxfounders, os_set_cmdtree);
	(void) command_add(&os_set_akicktime, os_set_cmdtree);
	(void) command_add(&os_set_spam, os_set_cmdtree);
	(void) command_add(&os_set_nickexpire, os_set_cmdtree);
	(void) command_add(&os_set_enforceprefix, os_set_cmdtree);

	(void) service_named_bind_command("operserv", &os_set);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("operserv", &os_set);

	(void) mowgli_patricia_destroy(os_set_cmdtree, &command_delete_trie_cb, os_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("operserv/set", MODULE_UNLOAD_CAPABILITY_OK)
