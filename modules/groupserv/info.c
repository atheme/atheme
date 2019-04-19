/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the GroupServ HELP command.
 */

#include <atheme.h>
#include "groupserv.h"

static void
gs_cmd_info(struct sourceinfo *si, int parc, char *parv[])
{
	struct mygroup *mg;
	struct tm *tm;
	char buf[BUFSIZE], strfbuf[BUFSIZE];
	struct metadata *md;

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INFO");
		command_fail(si, fault_needmoreparams, _("Syntax: INFO <!groupname>"));
		return;
	}

	if (!(mg = mygroup_find(parv[0])))
	{
		command_fail(si, fault_alreadyexists, _("Group \2%s\2 does not exist."), parv[0]);
		return;
	}

	tm = localtime(&mg->regtime);
	strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

	command_success_nodata(si, _("Information for \2%s\2:"), parv[0]);
	command_success_nodata(si, _("Registered  : %s (%s ago)"), strfbuf, time_ago(mg->regtime));

	if (config_options.show_entity_id || has_priv(si, PRIV_GROUP_AUSPEX))
		command_success_nodata(si, _("Entity ID   : %s"), entity(mg)->id);

	if (mg->flags & MG_PUBLIC || (si->smu != NULL && groupacs_sourceinfo_has_flag(mg, si, 0) && !groupacs_sourceinfo_has_flag(mg, si, GA_BAN)) || has_priv(si, PRIV_GROUP_AUSPEX))
		command_success_nodata(si, _("Founder     : %s"), mygroup_founder_names(mg));

	if ((md = metadata_find(mg, "description")))
		command_success_nodata(si, _("Description : %s"), md->value);
	if ((md = metadata_find(mg, "channel")))
		command_success_nodata(si, _("Channel     : %s"), md->value);
	if ((md = metadata_find(mg, "url")))
		command_success_nodata(si, _("URL         : %s"), md->value);
	if ((md = metadata_find(mg, "email")))
		command_success_nodata(si, _("Email       : %s"), md->value);

	*buf = '\0';

	if (mg->flags & MG_REGNOLIMIT)
		mowgli_strlcat(buf, "REGNOLIMIT", BUFSIZE);

	if (mg->flags & MG_ACSNOLIMIT)
	{
		if (*buf)
			mowgli_strlcat(buf, " ", BUFSIZE);

		mowgli_strlcat(buf, "ACSNOLIMIT", BUFSIZE);
	}

	if (mg->flags & MG_OPEN)
	{
		if (*buf)
			mowgli_strlcat(buf, " ", BUFSIZE);

		mowgli_strlcat(buf, "OPEN", BUFSIZE);
	}

	if (mg->flags & MG_PUBLIC)
	{
		if (*buf)
			mowgli_strlcat(buf, " ", BUFSIZE);

		mowgli_strlcat(buf, "PUBLIC", BUFSIZE);
	}

	if (*buf)
		command_success_nodata(si, _("Flags       : %s"), buf);

	if ((md = metadata_find(mg, "joinflags")) != NULL)
	{
		command_success_nodata(si, _("Join flags  : %s"), gflags_tostr(ga_flags, atoi(md->value)));
	}

	command_success_nodata(si, _("\2*** End of Info ***\2"));

	logcommand(si, CMDLOG_GET, "INFO: \2%s\2", parv[0]);
}

static struct command gs_info = {
	.name           = "INFO",
	.desc           = N_("Displays information about registered groups."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &gs_cmd_info,
	.help           = { .path = "groupserv/info" },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_info);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("groupserv", &gs_info);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/info", MODULE_UNLOAD_CAPABILITY_OK)
