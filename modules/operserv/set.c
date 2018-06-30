/*
 * Copyright (C) 2006 William Pitcock, et al.
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the OperServ SET command.
 */

#include "atheme.h"
#include <limits.h>

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

	(void) command_success_nodata(si, _("SET allows you to set various control flags\nfor services "
	                                    "that changes the way certain\noperations are performed."));

	(void) help_display_newline(si);

	(void) command_success_nodata(si, _("Note that all settings will be reset to the values\nin the "
	                                    "configuration file on rehash or services restart."));

	(void) help_display_newline(si);
	(void) command_help(si, os_set_cmdtree);
	(void) help_display_moreinfo(si, si->service, "SET");
	(void) help_display_suffix(si);
}

// SET <setting> <parameters>
static void
os_cmd_set(struct sourceinfo *si, int parc, char *parv[])
{
	char *setting = parv[0];
	struct command *c;

	if (setting == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <setting> <parameters>"));
		return;
	}

	// take the command through the hash table
        if ((c = command_find(os_set_cmdtree, setting)))
	{
		command_exec(si->service, si, c, parc - 1, parv + 1);
	}
	else
	{
		command_fail(si, fault_badparams, _("Invalid set command. Use \2/%s%s HELP SET\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", si->service->nick);
	}
}

static void
os_cmd_set_recontime(struct sourceinfo *si, int parc, char *parv[])
{
	char *recontime = parv[0];

	if (!recontime)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RECONTIME");
		command_fail(si, fault_needmoreparams, _("Syntax: SET RECONTIME <seconds>"));
		return;
	}

	int value = atoi(recontime);

	if (value < 0)
	{
		command_fail(si, fault_badparams, _("RECONTIME must be a positive integer, %s is invalid"), recontime);
		return;
	}
	else
	{
		me.recontime = value;
		command_success_nodata(si, "RECONTIME has been successfully set to %s seconds.", recontime);
		logcommand(si, CMDLOG_ADMIN, "SET:RECONTIME: \2%s\2", recontime);
	}
}

static void
os_cmd_set_maxlogins(struct sourceinfo *si, int parc, char *parv[])
{
	char *logins = parv[0];

	if (!logins)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MAXLOGINS");
		command_fail(si, fault_needmoreparams, _("Syntax: SET MAXLOGINS <value>"));
		return;
	}

	int value = atoi(logins);

	if (value < 3 || value > INT_MAX)
	{
		command_fail(si, fault_badparams, _("%s is invalid for MAXLOGINS value."), logins);
		return;
	}
	else
	{
		me.maxlogins = value;
		command_success_nodata(si, "MAXLOGINS has been successfully set to %s.", logins);
		logcommand(si, CMDLOG_ADMIN, "SET:MAXLOGINS: \2%s\2", logins);
	}
}

static void
os_cmd_set_maxusers(struct sourceinfo *si, int parc, char *parv[])
{
	char *users = parv[0];

	if (!users)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MAXUSERS");
		command_fail(si, fault_needmoreparams, _("Syntax: SET MAXUSERS <value>"));
		return;
	}

	int value = atoi(users);

	if (value < 0 || value > INT_MAX)
	{
		command_fail(si, fault_badparams, _("%s is invalid for MAXUSERS value."), users);
		return;
	}
	else
	{
		me.maxusers = value;
		command_success_nodata(si, "MAXUSERS has been successfully set to %s.", users);
		logcommand(si, CMDLOG_ADMIN, "SET:MAXUSERS: \2%s\2", users);
	}
}

static void
os_cmd_set_maxnicks(struct sourceinfo *si, int parc, char *parv[])
{
	char *nicks = parv[0];

	if (!nicks)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MAXNICKS");
		command_fail(si, fault_needmoreparams, _("Syntax: SET MAXNICKS <value>"));
		return;
	}

	int value = atoi(nicks);

	if (value < 1 || value > INT_MAX)
	{
		command_fail(si, fault_badparams, _("%s is invalid for MAXNICKS value."), nicks);
		return;
	}
	else
	{
		nicksvs.maxnicks = value;
		command_success_nodata(si, "MAXNICKS has been successfully set to %s.", nicks);
		logcommand(si, CMDLOG_ADMIN, "SET:MAXNICKS: \2%s\2", nicks);
	}
}

static void
os_cmd_set_maxchans(struct sourceinfo *si, int parc, char *parv[])
{
	char *chans = parv[0];

	if (!chans)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MAXCHANS");
		command_fail(si, fault_needmoreparams, _("Syntax: SET MAXCHANS <value>"));
		return;
	}

	int value = atoi(chans);

	if (value < 1 || value > INT_MAX)
	{
		command_fail(si, fault_badparams, _("%s is invalid for MAXCHANS value."), chans);
		return;
	}
	else
	{
		chansvs.maxchans = value;
		command_success_nodata(si, "MAXCHANS has been successfully set to %s.", chans);
		logcommand(si, CMDLOG_ADMIN, "SET:MAXCHANS: \2%s\2", chans);
	}
}

static void
os_cmd_set_mdlimit(struct sourceinfo *si, int parc, char *parv[])
{
	char *limit = parv[0];

	if (!limit)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MDLIMIT");
		command_fail(si, fault_needmoreparams, _("Syntax: SET MDLIMIT <value>"));
		return;
	}

	int value = atoi(limit);

	if (value < 1 || value > INT_MAX)
	{
		command_fail(si, fault_badparams, _("%s is invalid for MDLIMIT value."), limit);
		return;
	}
	else
	{
		me.mdlimit = value;
		command_success_nodata(si, "MDLIMIT has been successfully set to %s.", limit);
		logcommand(si, CMDLOG_ADMIN, "SET:MDLIMIT: \2%s\2", limit);
	}
}

static void
os_cmd_set_klinetime(struct sourceinfo *si, int parc, char *parv[])
{
	char *days = parv[0];

	if (!days)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "KLINETIME");
		command_fail(si, fault_needmoreparams, _("Syntax: SET KLINETIME <days>"));
		return;
	}

	int value = atoi(days);

	if (value < 0)
	{
		command_fail(si, fault_badparams, _("KLINETIME must be a positive integer, %s is invalid"), days);
		return;
	}
	else
	{
		unsigned int realvalue = value * 24 * 60 * 60;
		config_options.kline_time = realvalue;
		command_success_nodata(si, "KLINETIME has been successfully set to %s days.", days);
		logcommand(si, CMDLOG_ADMIN, "SET:KLINETIME: \2%s\2", days);
	}
}

static void
os_cmd_set_commitinterval(struct sourceinfo *si, int parc, char *parv[])
{
	char *minutes = parv[0];

	if (!minutes)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "COMMITINTERVAL");
		command_fail(si, fault_needmoreparams, _("Syntax: SET COMMITINTERVAL <minutes>"));
		return;
	}

	int value = atoi(minutes);

	if (value < 0)
	{
		command_fail(si, fault_badparams, _("COMMITINTERVAL must be a positive integer, %s is invalid"), minutes);
		return;
	}
	else
	{
		unsigned int realvalue = value * 60;
		config_options.commit_interval = realvalue;
		command_success_nodata(si, "COMMITINTERVAL has been successfully set to %s minutes.", minutes);
		logcommand(si, CMDLOG_ADMIN, "SET:COMMITINTERVAL: \2%s\2", minutes);
	}
}

static void
os_cmd_set_chanexpire(struct sourceinfo *si, int parc, char *parv[])
{
	char *days = parv[0];

	if (!days)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CHANEXPIRE");
		command_fail(si, fault_needmoreparams, _("Syntax: SET CHANEXPIRE <days>"));
		return;
	}

	int value = atoi(days);

	if (value < 0)
	{
		command_fail(si, fault_badparams, _("CHANEXPIRE must be a positive integer, %s is invalid"), days);
		return;
	}
	else
	{
		unsigned int realvalue = value * 24 * 60 * 60;
		chansvs.expiry = realvalue;
		command_success_nodata(si, "CHANEXPIRE has been successfully set to %s days.", days);
		logcommand(si, CMDLOG_ADMIN, "SET:CHANEXPIRE: \2%s\2", days);
	}
}

static void
os_cmd_set_maxchanacs(struct sourceinfo *si, int parc, char *parv[])
{
	char *chanacs = parv[0];

	if (!chanacs)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MAXCHANACS");
		command_fail(si, fault_needmoreparams, _("Syntax: SET MAXCHANACS <value>"));
		return;
	}

	int value = atoi(chanacs);

	if (value < 0 || value > INT_MAX)
	{
		command_fail(si, fault_badparams, _("%s is invalid for MAXCHANACS value."), chanacs);
		return;
	}
	else
	{
		chansvs.maxchanacs = value;
		command_success_nodata(si, "MAXCHANACS has been successfully set to %s.", chanacs);
		logcommand(si, CMDLOG_ADMIN, "SET:MAXCHANACS: \2%s\2", chanacs);
	}
}

static void
os_cmd_set_maxfounders(struct sourceinfo *si, int parc, char *parv[])
{
	char *founders = parv[0];

	if (!founders)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MAXFOUNDERS");
		command_fail(si, fault_needmoreparams, _("Syntax: SET MAXFOUNDERS <value>"));
		return;
	}

	int value = atoi(founders);

	/* Yes, I know how arbitrary the high value is, this is what it is in confprocess.c
	 * (I rounded it down though) -- JD
	 */
	if (value < 1 || value > 41)
	{
		command_fail(si, fault_badparams, _("%s is invalid for MAXFOUNDERS value."), founders);
		return;
	}
	else
	{
		chansvs.maxfounders = value;
		command_success_nodata(si, "MAXFOUNDERS has been successfully set to %s.", founders);
		logcommand(si, CMDLOG_ADMIN, "SET:MAXFOUNDERS: \2%s\2", founders);
	}
}

static void
os_cmd_set_akicktime(struct sourceinfo *si, int parc, char *parv[])
{
	char *minutes = parv[0];

	if (!minutes)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKICKTIME");
		command_fail(si, fault_needmoreparams, _("Syntax: SET AKICKTIME <minutes>"));
		return;
	}

	int value = atoi(minutes);

	if (value < 0)
	{
		command_fail(si, fault_badparams, _("AKICKTIME must be a positive integer, %s is invalid"), minutes);
		return;
	}
	else
	{
		unsigned int realvalue = value * 60;
		chansvs.akick_time = realvalue;
		command_success_nodata(si, "AKICKTIME has been successfully set to %s minutes.", minutes);
		logcommand(si, CMDLOG_ADMIN, "SET:AKICKTIME: \2%s\2", minutes);
	}
}

static void
os_cmd_set_spam(struct sourceinfo *si, int parc, char *parv[])
{
	char *spam = parv[0];

	if (!spam)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SPAM");
		command_fail(si, fault_needmoreparams, _("Syntax: SET SPAM <TRUE|FALSE>"));
		return;
	}

	if (!strcasecmp("TRUE", spam) || !strcasecmp("ON", spam))
	{
		if (nicksvs.spam)
		{
			command_fail(si, fault_badparams, _("SPAM directive is already set to %s."), spam);
			return;
		}

		nicksvs.spam = true;
		command_success_nodata(si, _("SPAM directive has been successfully set to %s."), spam);
		logcommand(si, CMDLOG_ADMIN, "SET:SPAM:TRUE");
		return;
	}
	else if (!strcasecmp("FALSE", spam) || !strcasecmp("OFF", spam))
	{
		if (!nicksvs.spam)
		{
			command_fail(si, fault_badparams, _("SPAM directive is already set to %s."), spam);
			return;
		}

		nicksvs.spam = false;
		command_success_nodata(si, _("SPAM directive has been successfully set to %s."), spam);
		logcommand(si, CMDLOG_ADMIN, "SET:SPAM:FALSE");
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SPAM");
		return;
	}
}

static void
os_cmd_set_nickexpire(struct sourceinfo *si, int parc, char *parv[])
{
	char *days = parv[0];

	if (!days)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NICKEXPIRE");
		command_fail(si, fault_needmoreparams, _("Syntax: SET NICKEXPIRE <days>"));
		return;
	}

	int value = atoi(days);

	if (value < 0)
	{
		command_fail(si, fault_badparams, _("NICKEXPIRE must be a positive integer, %s is invalid"), days);
		return;
	}
	else
	{
		unsigned int realvalue = value * 24 * 60 * 60;
		nicksvs.expiry = realvalue;
		command_success_nodata(si, "NICKEXPIRE has been successfully set to %s days.", days);
		logcommand(si, CMDLOG_ADMIN, "SET:NICKEXPIRE: \2%s\2", days);
	}
}

static void
os_cmd_set_enforceprefix(struct sourceinfo *si, int parc, char *parv[])
{
	char *prefix = parv[0];

	if (!prefix)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ENFORCEPREFIX");
		command_fail(si, fault_needmoreparams, _("Syntax: SET ENFORCEPREFIX <prefix>"));
		return;
	}

	nicksvs.enforce_prefix = sstrdup(prefix);
	command_success_nodata(si, "ENFORCEPREFIX has been successfully set to %s.", prefix);
	logcommand(si, CMDLOG_ADMIN, "SET:ENFORCEPREFIX: \2%s\2", prefix);
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
