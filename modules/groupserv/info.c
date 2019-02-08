/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include "atheme.h"

static const struct groupserv_core_symbols *gcsyms = NULL;

static const char *
gs_info_flags_str(const struct mygroup *const restrict mg)
{
	static const struct {
		const char *    name;
		unsigned int    flag;
	} flags_to_name[] = {
		{ "ACSNOLIMIT", MG_ACSNOLIMIT },
		{       "OPEN", MG_OPEN       },
		{     "PUBLIC", MG_PUBLIC     },
		{ "REGNOLIMIT", MG_REGNOLIMIT },
	};

	static char flagsbuf[BUFSIZE];

	(void) memset(flagsbuf, 0x00, sizeof flagsbuf);

	for (size_t i = 0; i < ((sizeof flags_to_name) / (sizeof flags_to_name[0])); i++)
	{
		if (! (mg->flags & flags_to_name[i].flag))
			continue;

		if (*flagsbuf)
			(void) mowgli_strlcat(flagsbuf, " ", sizeof flagsbuf);

		(void) mowgli_strlcat(flagsbuf, flags_to_name[i].name, sizeof flagsbuf);
	}

	if (! *flagsbuf)
		(void) mowgli_strlcat(flagsbuf, "NONE", sizeof flagsbuf);

	return flagsbuf;
}

static void
gs_cmd_info_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc != 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INFO");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: INFO <!group>"));
		return;
	}

	const char *const group = parv[0];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "INFO");
		(void) command_fail(si, fault_badparams, _("Syntax: INFO <!group>"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(group)))
	{
		command_fail(si, fault_alreadyexists, _("Group \2%s\2 does not exist."), group);
		return;
	}

	char strfbuf[BUFSIZE];
	struct tm *tm = localtime(&mg->regtime);

	(void) strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

	(void) command_success_nodata(si, _("Information for \2%s\2:"), group);
	(void) command_success_nodata(si, _("Registered  : %s (%s ago)"), strfbuf, time_ago(mg->regtime));

	if (config_options.show_entity_id || has_priv(si, PRIV_GROUP_AUSPEX))
		(void) command_success_nodata(si, _("Entity ID   : %s"), entity(mg)->id);

	if ((mg->flags & MG_PUBLIC) ||
	    has_priv(si, PRIV_GROUP_AUSPEX) ||
	    (
	        si->smu &&
	        gcsyms->groupacs_sourceinfo_has_flag(mg, si, 0) &&
	        ! gcsyms->groupacs_sourceinfo_has_flag(mg, si, GA_BAN)
	    ))
	{
		(void) command_success_nodata(si, _("Founder     : %s"), gcsyms->mygroup_founder_names(mg));
	}

	struct metadata *md;
	const char *const flagsbuf = gs_info_flags_str(mg);

	if ((md = metadata_find(mg, "channel")))
		(void) command_success_nodata(si, _("Channel     : %s"), md->value);

	if ((md = metadata_find(mg, "description")))
		(void) command_success_nodata(si, _("Description : %s"), md->value);

	if ((md = metadata_find(mg, "email")))
		(void) command_success_nodata(si, _("Email       : %s"), md->value);

	(void) command_success_nodata(si, _("Flags       : %s"), flagsbuf);

	if ((md = metadata_find(mg, "joinflags")))
		(void) command_success_nodata(si, _("Join Flags  : %s"), gflags_tostr(ga_flags, atoi(md->value)));

	if ((md = metadata_find(mg, "url")))
		(void) command_success_nodata(si, _("URL         : %s"), md->value);

	(void) command_success_nodata(si, _("\2*** End of Info ***\2"));
	(void) logcommand(si, CMDLOG_GET, "INFO: \2%s\2", group);
}

static struct command gs_cmd_info = {
	.name           = "INFO",
	.desc           = N_("Displays information about registered groups."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &gs_cmd_info_func,
	.help           = { .path = "groupserv/info" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");

	(void) service_bind_command(*gcsyms->groupsvs, &gs_cmd_info);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_unbind_command(*gcsyms->groupsvs, &gs_cmd_info);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/info", MODULE_UNLOAD_CAPABILITY_OK)
