/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains routines to handle the NickServ HELP command.
 */

#include <atheme.h>

#define SHORTHELP_CMDS_DEF      "ACCESS CERT DROP GHOST GROUP IDENTIFY INFO LISTCHANS LISTGROUPS LISTLOGINS " \
                                "LISTOWNMAIL LOGOUT REGAIN REGISTER RELEASE SENDPASS SET UNGROUP"

static char *shorthelp_cmds = NULL;

static void
ns_cmd_help(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char **const restrict parv)
{
	if (parv[0])
	{
		if (strcasecmp(parv[0], "COMMANDS") == 0)
		{
			(void) help_display_prefix(si, nicksvs.me);
			(void) command_help(si, nicksvs.me->commands);
			(void) help_display_moreinfo(si, nicksvs.me, NULL);
			(void) help_display_locations(si);
			(void) help_display_suffix(si);
		}
		else
			(void) help_display(si, nicksvs.me, parv[0], nicksvs.me->commands);

		return;
	}

	(void) help_display_prefix(si, nicksvs.me);

	if (nicksvs.no_nick_ownership)
	{
		(void) command_success_nodata(si, _("\2%s\2 allows users to \2'register'\2 an account for use with "
		                                    "\2%s\2."), nicksvs.nick, chansvs.nick);

		if (nicksvs.expiry > 0)
		{
			(void) help_display_newline(si);

			(void) command_success_nodata(si, _("If a registered account is not used by the owner for %u\n"
			                                    "days, \2%s\2 will drop the account, allowing it to be\n"
			                                    "re-registered."), (nicksvs.expiry / SECONDS_PER_DAY),
			                                    nicksvs.nick);
		}
	}
	else
	{
		(void) command_success_nodata(si, _("\2%s\2 allows users to \2'register'\2 a nickname, and stop\n"
		                                    "others from using that nick. \2%s\2 allows the owner of a\n"
		                                    "nickname to disconnect a user from the network that is using\n"
		                                    "their nickname."), nicksvs.nick, nicksvs.nick);

		if (nicksvs.expiry > 0)
		{
			(void) help_display_newline(si);

			(void) command_success_nodata(si, _("If a registered nick is not used by the owner for %u\n"
			                                    "days, \2%s\2 will drop the nickname, allowing it to be\n"
			                                    "re-registered."), (nicksvs.expiry / SECONDS_PER_DAY),
			                                    nicksvs.nick);
		}
	}

	(void) help_display_newline(si);

	if (shorthelp_cmds)
		(void) command_help_short(si, nicksvs.me->commands, shorthelp_cmds);
	else
		(void) command_help_short(si, nicksvs.me->commands, SHORTHELP_CMDS_DEF);

	(void) help_display_moreinfo(si, nicksvs.me, NULL);
	(void) help_display_verblist(si, nicksvs.me);
	(void) help_display_locations(si);
	(void) help_display_suffix(si);
}

static struct command ns_help = {
	.name           = "HELP",
	.desc           = STR_HELP_DESCRIPTION,
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_help,
	.help           = { .path = "help" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	(void) add_dupstr_conf_item("SHORTHELP", &nicksvs.me->conf_table, 0, &shorthelp_cmds, NULL);

	(void) service_named_bind_command("nickserv", &ns_help);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("nickserv", &ns_help);

	(void) del_conf_item("SHORTHELP", &nicksvs.me->conf_table);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/help", MODULE_UNLOAD_CAPABILITY_OK)
