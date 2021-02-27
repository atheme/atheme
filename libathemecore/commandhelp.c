/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * commandhelp.c: Help system implementation.
 */

#include <atheme.h>
#include "internal.h"

#define COMMAND_SHORTHELP_WRAP_COLS 64U

static unsigned int help_display_depth = 0;

static inline bool
can_execute_command(struct sourceinfo *const restrict si, const struct command *const restrict cmd)
{
	if (cmd->access != AC_NONE)
		if (! (has_priv(si, cmd->access) || (strcmp(cmd->access, AC_AUTHENTICATED) == 0 && si->smu)))
			return false;

	return true;
}

static void
help_not_available(struct sourceinfo *const restrict si, const char *const restrict cmd,
                   const char *const restrict subcmd, const bool cmd_exists)
{
	char buf[BUFSIZE];
	const char *text = cmd;

	if (subcmd)
	{
		(void) snprintf(buf, sizeof buf, "%s %s", subcmd, cmd);
		text = buf;
	}

	if (cmd_exists)
		(void) command_fail(si, fault_nosuch_target, _("No help available for \2%s\2."), text);
	else
		(void) command_fail(si, fault_nosuch_target, _("No such command \2%s\2."), cmd);

	(void) help_display_newline(si);
	(void) help_display_locations(si);
}

static bool
help_evaluate_condition(struct sourceinfo *const restrict si, const char *restrict str)
{
	while (*str == ' ' || *str == '\t')
		str++;

	if (! *str)
	{
		(void) slog(LG_DEBUG, "%s: empty condition", MOWGLI_FUNC_NAME);
		return false;
	}

	if (*str == '!')
		return !help_evaluate_condition(si, str + 1);

	char condition[BUFSIZE];

	(void) mowgli_strlcpy(condition, str, sizeof condition);

	char *arg = strpbrk(condition, " \t");

	if (arg)
	{
		while (*arg == ' ' || *arg == '\t')
			*arg++ = 0x00;

		if (*arg)
		{
			char *const end = strpbrk(arg, " \t");

			if (end)
				*end = 0x00;

			if (strcasecmp(condition, "module") == 0)
				return (module_find_published(arg) != NULL);

			if (strcasecmp(condition, "priv") == 0)
				return has_priv(si, arg);
		}
	}

	if (strcasecmp(condition, "anyprivs") == 0)
		return has_any_privs(si);

	if (strcasecmp(condition, "auth") == 0)
		return (me.auth != AUTH_NONE);

	if (strcasecmp(condition, "halfops") == 0)
		return ircd->uses_halfops;

	if (strcasecmp(condition, "owner") == 0)
		return ircd->uses_owner;

	if (strcasecmp(condition, "protect") == 0)
		return ircd->uses_protect;

	(void) slog(LG_DEBUG, "%s: unrecognised condition '%s' (string '%s')", MOWGLI_FUNC_NAME, condition, str);
	return false;
}

static void
help_display_path(struct sourceinfo *const restrict si, const char *const restrict cmd,
                  const char *const restrict path, const char *const restrict service_name)
{
	char fullpath[PATH_MAX];
	FILE *fh = NULL;

	if (*path == '/')
	{
		(void) mowgli_strlcpy(fullpath, path, sizeof fullpath);

		if (! (fh = fopen(fullpath, "r")))
			(void) slog(LG_DEBUG, "%s: fopen('%s'): %s", MOWGLI_FUNC_NAME, fullpath, strerror(errno));
	}
	else
	{
		char subname[BUFSIZE];

		(void) mowgli_strlcpy(subname, path, sizeof subname);

		if (nicksvs.no_nick_ownership && strncmp(subname, "nickserv/", 9) == 0)
			(void) memcpy(subname, "user", 4);

		const char *lang = NULL;

		if (si->smu && (lang = language_get_real_name(si->smu->language)) && strcasecmp(lang, "en") == 0)
			lang = NULL;

		if (lang)
		{
			(void) snprintf(fullpath, sizeof fullpath, "%s/help/%s/%s", SHAREDIR, lang, subname);

			if (! (fh = fopen(fullpath, "r")))
				(void) slog(LG_DEBUG, "%s: fopen('%s'): %s", MOWGLI_FUNC_NAME,
				                      fullpath, strerror(errno));
		}

		if (! fh)
		{
			(void) snprintf(fullpath, sizeof fullpath, "%s/help/%s", SHAREDIR, subname);

			if (! (fh = fopen(fullpath, "r")))
				(void) slog(LG_DEBUG, "%s: fopen('%s'): %s", MOWGLI_FUNC_NAME,
				                      fullpath, strerror(errno));
		}
	}

	if (! fh)
	{
		(void) command_fail(si, fault_nosuch_target, _("Could not open help file for \2%s\2."), cmd);
		(void) help_display_newline(si);
		(void) help_display_locations(si);
		return;
	}

	unsigned int ifnest_false = 0;
	unsigned int ifnest = 0;
	char buf[BUFSIZE];

	while (fgets(buf, sizeof buf, fh))
	{
		(void) strip(buf);
		(void) replace(buf, sizeof buf, "&nick&", service_name);

		if (strncasecmp(buf, "#if", 3) == 0)
		{
			if (ifnest_false || ! help_evaluate_condition(si, buf + 3))
				ifnest_false++;

			ifnest++;
			continue;
		}

		if (strncasecmp(buf, "#endif", 6) == 0)
		{
			if (ifnest_false)
				ifnest_false--;

			if (ifnest)
				ifnest--;

			continue;
		}

		if (strncasecmp(buf, "#else", 5) == 0)
		{
			if (ifnest && ifnest_false < 2)
				ifnest_false ^= 1;

			continue;
		}

		if (ifnest_false)
			continue;

		if (*buf)
			(void) command_success_nodata(si, "%s", buf);
		else
			(void) help_display_newline(si);
	}

	if (ferror(fh))
		(void) slog(LG_DEBUG, "%s: fgets('%s'): %s", MOWGLI_FUNC_NAME, fullpath, strerror(errno));

	(void) fclose(fh);

	(void) help_display_newline(si);
}

void
help_display_as_subcmd(struct sourceinfo *const restrict si, const struct service *const restrict service,
                       const char *const restrict subcmd, const char *const restrict cmd,
                       mowgli_patricia_t *const restrict cmd_list)
{
	char ccmd[BUFSIZE];

	(void) mowgli_strlcpy(ccmd, cmd, sizeof ccmd);

	char *delim = strchr(ccmd, ' ');

	if (delim)
		*delim++ = 0x00;

	(void) help_display_prefix(si, service);

	const struct command *const command = mowgli_patricia_retrieve(cmd_list, ccmd);

	if (command)
	{
		if (command->help.path)
			(void) help_display_path(si, cmd, command->help.path, service->disp);
		else if (command->help.func)
			(void) command->help.func(si, delim);
		else
			(void) help_not_available(si, cmd, subcmd, true);
	}
	else
		(void) help_not_available(si, cmd, subcmd, false);

	(void) help_display_suffix(si);
}

void
help_display(struct sourceinfo *const restrict si, const struct service *const restrict service,
             const char *const restrict cmd, mowgli_patricia_t *const restrict cmd_list)
{
	(void) help_display_as_subcmd(si, service, NULL, cmd, cmd_list);
}

void
help_display_invalid(struct sourceinfo *const restrict si, const struct service *const restrict service,
                     const char *const restrict subcmd)
{
	if (subcmd && *subcmd)
		(void) command_fail(si, fault_badparams, _("Invalid \2%s\2 \2%s\2 subcommand."), service->nick, subcmd);
	else
		(void) command_fail(si, fault_badparams, _("Invalid \2%s\2 command."), service->nick);

	if (! command_find(service->commands, "HELP"))
		/* Don't tell people to use `/msg foo HELP' if the respective service help module isn't loaded
		 *   -- amdj & ilbelkyr
		 */
		return;

	if (subcmd && *subcmd)
		(void) command_fail(si, fault_badparams, _("Use \2/msg %s HELP %s\2 for a \2%s\2 \2%s\2 subcommand "
		                                           "listing."), service->disp, subcmd, service->nick, subcmd);
	else
		(void) command_fail(si, fault_badparams, _("Use \2/msg %s HELP\2 for a \2%s\2 command listing."),
		                                           service->disp, service->nick);
}

void
help_display_locations(struct sourceinfo *const restrict si)
{
	const char *const helpchan = config_options.helpchan;
	const char *const helpurl = config_options.helpurl;

	if (helpchan && helpurl && *helpchan && *helpurl)
	{
		(void) command_success_nodata(si, _("If you're having trouble, or you need some additional help,\n"
		                                    "you may wish to join the help channel \2%s\2 or visit the\n"
		                                    "help webpage \2%s\2"), helpchan, helpurl);
		(void) help_display_newline(si);
	}
	else if (helpchan && *helpchan)
	{
		(void) command_success_nodata(si, _("If you're having trouble, or you need some additional help,\n"
		                                    "you may wish to join the help channel \2%s\2"), helpchan);
		(void) help_display_newline(si);
	}
	else if (helpurl && *helpurl)
	{
		(void) command_success_nodata(si, _("If you're having trouble, or you need some additional help,\n"
		                                    "you may wish to visit the help webpage\n\2%s\2"), helpurl);
		(void) help_display_newline(si);
	}
}

void
help_display_moreinfo(struct sourceinfo *const restrict si, const struct service *const restrict service,
                      const char *const restrict subcmd_of)
{
	if (subcmd_of && *subcmd_of)
		(void) command_success_nodata(si, _("For more information on a \2%s\2 \2%s\2 subcommand, type:\n"
		                                    "\2/msg %s HELP %s <subcommand>\2"), service->nick, subcmd_of,
		                                    service->disp, subcmd_of);
	else
		(void) command_success_nodata(si, _("For more information on a \2%s\2 command, type:\n"
		                                    "\2/msg %s HELP <command>\2"), service->nick, service->disp);

	(void) help_display_newline(si);
}

void
help_display_newline(struct sourceinfo *const restrict si)
{
	(void) command_success_nodata(si, " ");
}

void
help_display_prefix(struct sourceinfo *const restrict si, const struct service *const restrict service)
{
	if (++help_display_depth == 1)
	{
		(void) command_success_nodata(si, _("***** \2%s Help\2 *****"), service->nick);
		(void) help_display_newline(si);
	}
}

void
help_display_suffix(struct sourceinfo *const restrict si)
{
	if (--help_display_depth == 0)
		(void) command_success_nodata(si, _("***** \2End of Help\2 *****"));
}

void
help_display_verblist(struct sourceinfo *const restrict si, const struct service *const restrict service)
{
	(void) command_success_nodata(si, _("For a verbose listing of all \2%s\2 commands, type:"), service->nick);
	(void) command_success_nodata(si, "\2/msg %s HELP COMMANDS\2", service->disp);
	(void) help_display_newline(si);
}

void
command_help(struct sourceinfo *const restrict si, mowgli_patricia_t *const restrict commandtree)
{
	mowgli_patricia_iteration_state_t state;
	struct command *cmd;

	bool cmds_displayed = false;

	MOWGLI_PATRICIA_FOREACH(cmd, &state, commandtree)
	{
		if (! can_execute_command(si, cmd))
			continue;

		if (! cmds_displayed)
		{
			if (! si->service || si->service->commands == commandtree)
				(void) command_success_nodata(si, _("The following commands are available:"));
			else
				(void) command_success_nodata(si, _("The following subcommands are available:"));

			(void) help_display_newline(si);

			cmds_displayed = true;
		}

		(void) command_success_nodata(si, "  \2%-15s\2 %s", cmd->name, translation_get(_(cmd->desc)));
	}

	if (! cmds_displayed && (! si->service || si->service->commands == commandtree))
		(void) command_fail(si, fault_noprivs, _("You do not have the privileges to execute any commands."));
	else if (! cmds_displayed)
		(void) command_fail(si, fault_noprivs, _("You do not have the privileges to execute any subcommands."));

	(void) help_display_newline(si);
}

void
command_help_short(struct sourceinfo *const restrict si, mowgli_patricia_t *const restrict commandtree,
                   const char *restrict shortlist)
{
	mowgli_patricia_iteration_state_t state;
	struct command *cmd;

	char buf[BUFSIZE] = { 0x00 };
	size_t buflen = 0;

	bool any_full_output_done = false;
	bool any_short_output_done = false;

	while (shortlist && *shortlist && isspace(*shortlist))
		shortlist++;

	if (! (shortlist && *shortlist))
		goto additional;

	MOWGLI_PATRICIA_FOREACH(cmd, &state, commandtree)
	{
		if (! can_execute_command(si, cmd))
			continue;

		// Only display full description for commands in the shorthelp description list
		if (! string_in_list(cmd->name, shortlist))
			continue;

		if (! any_full_output_done)
		{
			if (! si->service || si->service->commands == commandtree)
				(void) command_success_nodata(si, _("The following commands are available:"));
			else
				(void) command_success_nodata(si, _("The following subcommands are available:"));

			(void) help_display_newline(si);

			any_full_output_done = true;
		}

		(void) command_success_nodata(si, "  \2%-15s\2 %s", cmd->name, translation_get(_(cmd->desc)));
	}

additional:

	MOWGLI_PATRICIA_FOREACH(cmd, &state, commandtree)
	{
		if (! can_execute_command(si, cmd))
			continue;

		// If the command is in the shorthelp description list then it was already shown to the user above
		if (string_in_list(cmd->name, shortlist))
			continue;

		if (! any_short_output_done)
		{
			if (any_full_output_done && (! si->service || si->service->commands == commandtree))
			{
				(void) help_display_newline(si);
				(void) command_success_nodata(si, _("The following additional commands are available:"));
			}
			else if (any_full_output_done)
			{
				(void) help_display_newline(si);
				(void) command_success_nodata(si, _("The following additional subcommands are available:"));
			}
			else if (! si->service || si->service->commands == commandtree)
				(void) command_success_nodata(si, _("The following commands are available:"));
			else
				(void) command_success_nodata(si, _("The following subcommands are available:"));

			(void) help_display_newline(si);

			any_short_output_done = true;
		}

		const size_t cmdlen = strlen(cmd->name);

		if (*buf)
		{
			(void) mowgli_strlcat(buf, ",", sizeof buf);

			buflen++;

			if ((buflen + cmdlen) > COMMAND_SHORTHELP_WRAP_COLS)
			{
				(void) command_success_nodata(si, "  %s", buf);

				buf[0x00] = 0x00;
				buflen = 0;
			}
			else
			{
				(void) mowgli_strlcat(buf, " ", sizeof buf);

				buflen++;
			}
		}

		(void) mowgli_strlcat(buf, "\2", sizeof buf);
		(void) mowgli_strlcat(buf, cmd->name, sizeof buf);
		(void) mowgli_strlcat(buf, "\2", sizeof buf);

		buflen += cmdlen + 2;
	}

	if (buflen)
		(void) command_success_nodata(si, "  %s", buf);

	if (! any_full_output_done && ! any_short_output_done)
	{
		if (! si->service || si->service->commands == commandtree)
			(void) command_fail(si, fault_noprivs, _("You do not have the privileges to execute any commands."));
		else
			(void) command_fail(si, fault_noprivs, _("You do not have the privileges to execute any subcommands."));
	}

	(void) help_display_newline(si);
}
