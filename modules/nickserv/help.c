/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the NickServ HELP command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/help", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_help = { "HELP", N_("Displays contextual help information."), AC_NONE, 1, ns_cmd_help, { .path = "help" } };

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_help);
}

void _moddeinit()
{
	service_named_unbind_command("nickserv", &ns_help);
}

/* HELP <command> [params] */
void ns_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), nicksvs.nick);
		if (nicksvs.no_nick_ownership)
		{
			command_success_nodata(si, _("\2%s\2 allows users to \2'register'\2 an account for use with \2%s\2."), nicksvs.nick, chansvs.nick);
			if (nicksvs.expiry > 0)
			{
				command_success_nodata(si, _("If a registered account is not used by the owner for %d days,\n"
							"\2%s\2 will drop the account, allowing it to be reregistered."),
						(nicksvs.expiry / 86400), nicksvs.nick);
			}
		}
		else
		{
			command_success_nodata(si, _("\2%s\2 allows users to \2'register'\2 a nickname, and stop\n"
						"others from using that nick. \2%s\2 allows the owner of a\n"
						"nickname to disconnect a user from the network that is using\n"
						"their nickname."), nicksvs.nick, nicksvs.nick);
			if (nicksvs.expiry > 0)
			{
				command_success_nodata(si, _("If a registered nick is not used by the owner for %d days,\n"
							"\2%s\2 will drop the nickname, allowing it to be reregistered."),
						(nicksvs.expiry / 86400), nicksvs.nick);
			}
		}
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information on a command, type:"));
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == false) ? "msg " : "", nicksvs.me->disp);
		command_success_nodata(si, _("For a verbose listing of all commands, type:"));
		command_success_nodata(si, "\2/%s%s help commands\2", (ircd->uses_rcommand == false) ? "msg " : "", nicksvs.me->disp);
		command_success_nodata(si, " ");

		command_help_short(si, si->service->commands, "REGISTER IDENTIFY GHOST RELEASE INFO LISTCHANS SET GROUP UNGROUP FDROP FUNGROUP MARK FREEZE SENDPASS VHOST");

		command_success_nodata(si, _("***** \2End of Help\2 *****"));
		
		/* Fun for helpchan/helpurl. */
		if (config_options.helpchan && config_options.helpurl)
			command_success_nodata(si, _("If you're having trouble or you need some additional help, you may want to join the help channel %s or visit the help webpage %s"), 
					config_options.helpchan, config_options.helpurl);
		else if (config_options.helpchan && !config_options.helpurl)
			command_success_nodata(si, _("If you're having trouble or you need some additional help, you may want to join the help channel %s"), config_options.helpchan);
		else if (!config_options.helpchan && config_options.helpurl)
			command_success_nodata(si, _("If you're having trouble or you need some additional help, you may want to visit the help webpage %s"), config_options.helpurl);
		
		return;
	}

	if (!strcasecmp("COMMANDS", command))
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), nicksvs.nick);
		command_help(si, si->service->commands);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
		return;
	}

	/* take the command through the hash table */
	help_display(si, si->service, command, si->service->commands);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
