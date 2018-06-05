/*
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the ChanServ HELP command.
 */

#include "atheme.h"

#define SHORTHELP_CMDS_DEF      "AKICK BAN CLEAR DEOP DEVOICE DROP FLAGS GETKEY INFO INVITE KICK KICKBAN OP " \
                                "QUIET REGISTER SET TOPIC UNBAN UNQUIET VOICE WHY"

static char *shorthelp_cmds = NULL;

static void
cs_cmd_help(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char **const restrict parv)
{
	if (parv[0])
	{
		const char *command = parv[0];

		// Advance past the prefix and HELP command text, & channel name, for fantasy commands
		if (si->c && (command = strchr(command, ' ')))
			command++;

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

	(void) command_success_nodata(si, _("\2%s\2 gives normal users the ability to maintain control\nof a "
	                                    "channel, without the need of a bot. Channel takeovers are\nvirtually "
	                                    "impossible when a channel is registered with \2%s\2.\n \nRegistration "
	                                    "is a quick and painless process. Once registered,\nthe founder can "
	                                    "maintain complete and total control over the channel."), chansvs.nick,
	                                    chansvs.nick);

	(void) help_display_newline(si);

	if (chansvs.expiry > 0)
		(void) command_success_nodata(si, _("Please note that channels will expire after %d days of "
		                                    "inactivity,\nor if there are no eligible channel successors.\n"
		                                    " \nActivity is defined as a user with one of %s being on the "
		                                    "channel."), (chansvs.expiry / 86400),
		                                    bitmask_to_flags2(CA_USEDUPDATE & ca_all, 0));
	else
		(void) command_success_nodata(si, _("Please note that channels will expire if there are no eligible "
		                                    "channel successors."));

	(void) help_display_newline(si);

	(void) command_success_nodata(si, _("Successors are primarily those who have the +S (if available) or +R "
	                                    "flag\nset on their account in the channel, although other people may "
	                                    "be chosen\ndepending on their access level and activity."));

	if (config_options.join_chans && chansvs.fantasy && *chansvs.trigger)
	{
		(void) help_display_newline(si);

		(void) command_success_nodata(si, _("Commands can also be given on channel by prefixing one of '%s'"
		                                    "\nand omitting the channel name. These are called \"fantasy\""
		                                    "\ncommands and can also be disabled on a per-channel basis."),
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
	.desc           = N_("Displays contextual help information."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &cs_cmd_help,
	.help           = { .path = "help" },
};

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
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
