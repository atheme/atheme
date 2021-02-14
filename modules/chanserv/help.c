/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains routines to handle the ChanServ HELP command.
 */

#include <atheme.h>

#define SHORTHELP_CMDS_DEF      "AKICK BAN CLEAR DEOP DEVOICE DROP FLAGS GETKEY INFO INVITE KICK KICKBAN OP " \
                                "QUIET REGISTER SET TOPIC UNBAN UNQUIET VOICE WHY"

static char *shorthelp_cmds = NULL;

static void
cs_cmd_help(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char **const restrict parv)
{
	const char *command = parv[0];

	if (si->c && command)
	{
		// Advance past the prefix and HELP command text, & channel name, for fantasy commands
		command = strchr(command, ' ');
		if (command)
			command++;
	}

	if (command)
	{
		if (strcasecmp(command, "COMMANDS") == 0)
		{
			(void) help_display_prefix(si, chansvs.me);
			(void) command_help(si, chansvs.me->commands);
			(void) help_display_moreinfo(si, chansvs.me, NULL);
			(void) help_display_locations(si);
			(void) help_display_suffix(si);
		}
		else
			(void) help_display(si, chansvs.me, command, chansvs.me->commands);

		return;
	}

	(void) help_display_prefix(si, chansvs.me);

	(void) command_success_nodata(si, _("\2%s\2 gives normal users the ability to maintain control\n"
	                                    "of a channel, without the need of a bot. Channel takeovers are\n"
	                                    "virtually impossible when a channel is registered with \2%s\2."),
	                                    chansvs.nick, chansvs.nick);

	(void) help_display_newline(si);

	(void) command_success_nodata(si, _("Registration is a quick and painless process. Once registered,\n"
	                                    "the founder can maintain complete and total control over the\n"
	                                    "channel."));

	(void) help_display_newline(si);

	if (chansvs.expiry > 0)
	{
		(void) command_success_nodata(si, _("Please note that channels will expire after %u days of inactivity,\n"
		                                    "or if there are no eligible channel successors."),
		                                    (chansvs.expiry / SECONDS_PER_DAY));

		(void) help_display_newline(si);

		(void) command_success_nodata(si, _("Activity is defined as a user with one of \2%s\2 being on the "
		                                    "channel."), bitmask_to_flags2(CA_USEDUPDATE & ca_all, 0));
	}
	else
		(void) command_success_nodata(si, _("Please note that channels will expire if there are no eligible "
		                                    "channel successors."));

	(void) help_display_newline(si);

	(void) command_success_nodata(si, _("Successors are primarily those who have the \2+S\2 (if available)\n"
	                                    "or \2+R\2 flag set on their account in the channel, although other\n"
	                                    "people may be chosen depending on their access level and activity."));

	if (config_options.join_chans && chansvs.fantasy && *chansvs.trigger)
	{
		(void) help_display_newline(si);

		(void) command_success_nodata(si, _("Commands can also be given on channel by prefixing one of '%s'\n"
		                                    "and omitting the channel name. These are called \"fantasy\"\n"
		                                    "commands and can also be disabled on a per-channel basis."),
		                                    chansvs.trigger);
	}

	(void) help_display_newline(si);

	if (shorthelp_cmds)
		(void) command_help_short(si, chansvs.me->commands, shorthelp_cmds);
	else
		(void) command_help_short(si, chansvs.me->commands, SHORTHELP_CMDS_DEF);

	(void) help_display_moreinfo(si, chansvs.me, NULL);
	(void) help_display_verblist(si, chansvs.me);
	(void) help_display_locations(si);
	(void) help_display_suffix(si);
}

static struct command cs_help = {
	.name           = "HELP",
	.desc           = STR_HELP_DESCRIPTION,
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &cs_cmd_help,
	.help           = { .path = "help" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	(void) add_dupstr_conf_item("SHORTHELP", &chansvs.me->conf_table, 0, &shorthelp_cmds, NULL);

	(void) service_named_bind_command("chanserv", &cs_help);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("chanserv", &cs_help);

	(void) del_conf_item("SHORTHELP", &chansvs.me->conf_table);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/help", MODULE_UNLOAD_CAPABILITY_OK)
