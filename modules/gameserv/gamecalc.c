/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 William Pitcock <nenolod@nenolod.net>
 * Copyright (C) 2006-2010 Jilles Tjoelker <jilles@stack.nl>
 *
 * Dice generator.
 */

#include <atheme.h>
#include "gameserv_common.h"

static const char *df_dice_table[3] = { "[-]", "[ ]", "[+]" };

static void
command_wod(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	char *arg_dice, *arg_difficulty;
	unsigned int ii = 0;
	unsigned int dice, difficulty;
	unsigned int roll, total, roll_count = 0, i;
	unsigned int success = 0, failure = 0, botches = 0, rerolls = 0;
	static char buf[BUFSIZE];
	char *end_p;

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "WOD");
		command_fail(si, fault_needmoreparams, _("Syntax: WOD <dice> <difficulty>"));
		return;
	}

	arg_dice = parv[ii++];
	arg_difficulty = parv[ii++];

	while (roll_count < 3 && arg_dice != NULL && arg_difficulty != NULL)
	{
		success = 0;
		failure = 0;
		botches = 0;
		rerolls = 0;
		roll_count++;

		if (! string_to_uint(arg_dice, &dice) || dice > 30 || dice < 1)
		{
			command_fail(si, fault_badparams, _("Only 1-30 dice may be thrown at one time."));
			return;
		}
		else if (! string_to_uint(arg_difficulty, &difficulty) || difficulty > 10 || difficulty < 1)
		{
			command_fail(si, fault_badparams, _("Difficulty setting must be between 1 and 10."));
			return;
		}
		else
		{
			end_p = buf;

			for (i = 0; i < dice; i++)
			{
				roll = atheme_random_uniform(10) + 1;

				end_p += snprintf(end_p, BUFSIZE - (end_p - buf), "%u  ", roll);

				if (roll == 1)
				{
					botches++;
					continue;
				}
				else if (roll == 10)
					rerolls++;

				if (roll >= difficulty)
					success++;
				else
					failure++;
			}

			rerolls = rerolls - botches;
			total = success - botches;

			gs_command_report(si, _("%s rolls %u dice at difficulty %u: %s"), si->su->nick, dice, difficulty, buf);

			if (rerolls > 0)
				gs_command_report(si, _("Successes: %u, Failures: %u, Botches: %u, Total: %u. You may reroll %u if you have a specialty."),
					success, failure, botches, total, rerolls);
			else
				gs_command_report(si, _("Successes: %u, Failures: %u, Botches: %u, Total: %u."),
					success, failure, botches, total);
		}

		// prepare for another go.
		arg_dice = parv[ii++];
		arg_difficulty = parv[ii++];
	}
}

static void
command_nwod(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	char *arg_dice, *arg_rerollflag;
	unsigned int ii = 0;
	unsigned int dice, reroll;
	unsigned int roll, total, roll_count = 0, i;
	unsigned int success = 0, failure = 0, botches = 0, rerolls = 0;
	static char buf[BUFSIZE];
	char *end_p;

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;
	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NWOD");
		command_fail(si, fault_needmoreparams, _("Syntax: NWOD [-chance] <dice> [-reroll] [reroll]"));
		return;
	}
	arg_dice = parv[ii++];
	arg_rerollflag = parv[ii++];

	while (roll_count < 3 && arg_dice != NULL)
	{
		success = 0;
		failure = 0;
		botches = 0;
		rerolls = 0;
		roll_count++;

		if (strcasecmp(arg_dice, "-chance") == 0)
		{
			roll = atheme_random_uniform(10) + 1;

			if (roll == 1)
				botches++;

			if (roll == 10)
				success++;

			gs_command_report(si, _("%s rolls a chance die: %u"), si->su->nick, roll);
			gs_command_report(si, _("Successes: %u, Failures: %u, Botches: %u."), success, failure, botches);
			return;
		}

		if (! string_to_uint(arg_dice, &dice) || dice > 30 || dice < 1)
		{
			command_fail(si, fault_badparams, _("Only 1-30 dice may be thrown at one time."));
			return;
		}

		if (arg_rerollflag != NULL && strcasecmp(arg_rerollflag, "-reroll") == 0 && parv[ii + 1] != NULL)
		{
			if (! string_to_uint(parv[ii++], &reroll))
			{
				command_fail(si, fault_badparams, _("Invalid option for \2%s\2"), arg_rerollflag);
				return;
			}
		}
		else
			reroll = 10;

		{
			end_p = buf;

			for (i = 0; i < dice; i++)
			{
				roll = atheme_random_uniform(10) + 1;

				end_p += snprintf(end_p, BUFSIZE - (end_p - buf), "%u  ", roll);

				if (roll == 1)
				{
					botches++;
					continue;
				}
				else if (roll >= reroll)
					rerolls++;

				if (roll >= 8)
					success++;
				else
					failure++;
			}

			rerolls = rerolls - botches;
			total = success - botches;

			gs_command_report(si, _("%s rolls %u dice: %s"), si->su->nick, dice, buf);

			if (rerolls > 0)
				gs_command_report(si, _("Successes: %u, Failures: %u, Botches: %u, Total: %u. You may reroll %u."),
					success, failure, botches, total, rerolls);
			else
				gs_command_report(si, _("Successes: %u, Failures: %u, Botches: %u, Total: %u."),
					success, failure, botches, total);
		}

		// prepare for another go.
		arg_dice = parv[ii++];
		arg_rerollflag = parv[ii++];
	}
}

static void
command_df(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	char buf[BUFSIZE];
	unsigned int i, dice;

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DF");
		command_fail(si, fault_needmoreparams, _("Syntax: DF <dice>"));
		return;
	}

	if (! string_to_uint(parv[0], &dice) || dice > 30 || dice < 1)
	{
		command_fail(si, fault_badparams, _("Only 1-30 dice may be thrown at one time."));
		return;
	}

	*buf = '\0';

	for (i = 0; i < dice; i++)
	{
		int roll = atheme_random() % 3;

		if (*buf != '\0')
			mowgli_strlcat(buf, df_dice_table[roll], BUFSIZE);
		else
			mowgli_strlcpy(buf, df_dice_table[roll], BUFSIZE);
	}

	gs_command_report(si, _("Result: %s"), buf);
}

static struct command cmd_wod = {
	.name           = "WOD",
	.desc           = N_("WOD-style dice generation."),
	.access         = AC_NONE,
	.maxparc        = 7,
	.cmd            = &command_wod,
	.help           = { .path = "gameserv/roll" },
};

static struct command cmd_nwod = {
	.name           = "NWOD",
	.desc           = N_("New WOD-style dice generation."),
	.access         = AC_NONE,
	.maxparc        = 7,
	.cmd            = &command_nwod,
	.help           = { .path = "gameserv/roll" },
};

static struct command cmd_df = {
	.name           = "DF",
	.desc           = N_("Fudge-style dice generation."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &command_df,
	.help           = { .path = "gameserv/roll" },
};

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("gameserv", &cmd_wod);
	service_named_bind_command("gameserv", &cmd_nwod);
	service_named_bind_command("gameserv", &cmd_df);

	service_named_bind_command("chanserv", &cmd_wod);
	service_named_bind_command("chanserv", &cmd_nwod);
	service_named_bind_command("chanserv", &cmd_df);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("gameserv", &cmd_wod);
	service_named_unbind_command("gameserv", &cmd_nwod);
	service_named_unbind_command("gameserv", &cmd_df);

	service_named_unbind_command("chanserv", &cmd_wod);
	service_named_unbind_command("chanserv", &cmd_nwod);
	service_named_unbind_command("chanserv", &cmd_df);
}

SIMPLE_DECLARE_MODULE_V1("gameserv/gamecalc", MODULE_UNLOAD_CAPABILITY_OK)
