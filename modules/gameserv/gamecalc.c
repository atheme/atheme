/*
 * Copyright (c) 2005-2007 William Pitcock <nenolod@nenolod.net>
 * Copyright (c) 2006-2007 Jilles Tjoelker <jilles@stack.nl>
 *
 * Rights to this code are documented in doc/LICENSE.
 *
 * Dice generator.
 */

#include "atheme.h"
#include "gameserv_common.h"

static const char *df_dice_table[3] = { "[-]", "[ ]", "[+]" };

static void
command_wod(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	char *arg_dice, *arg_difficulty;
	int ii = 0;
	int dice, difficulty;
	int roll, total, roll_count = 0, i;
	int success = 0, failure = 0, botches = 0, rerolls = 0;
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

		dice = atoi(arg_dice);
		difficulty = atoi(arg_difficulty);

		if (dice > 30 || dice < 1)
		{
			command_fail(si, fault_badparams, _("Only 1-30 dice may be thrown at one time."));
			return;
		}
		else if (difficulty > 10 || difficulty < 1)
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

				end_p += snprintf(end_p, BUFSIZE - (end_p - buf), "%d  ", roll);

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

			gs_command_report(si, _("%s rolls %d dice at difficulty %d: %s"), si->su->nick, dice, difficulty, buf);

			if (rerolls > 0)
				gs_command_report(si, _("Successes: %d, Failures: %d, Botches: %d, Total: %d. You may reroll %d if you have a specialty."),
					success, failure, botches, total, rerolls);
			else
				gs_command_report(si, _("Successes: %d, Failures: %d, Botches: %d, Total: %d."),
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
	int ii = 0;
	int dice, reroll;
	int roll, total, roll_count = 0, i;
	int success = 0, failure = 0, botches = 0, rerolls = 0;
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

		dice = atoi(arg_dice);

		if (dice == 0 && !strcasecmp(arg_dice, "-chance"))
		{
			roll = atheme_random_uniform(10) + 1;

			if (roll == 1)
				botches++;

			if (roll == 10)
				success++;

			gs_command_report(si, _("%s rolls a chance die: %d"), si->su->nick, roll);
			gs_command_report(si, _("Successes: %d, Failures: %d, Botches: %d."), success, failure, botches);
			return;
		}

		if (dice > 30 || dice < 1)
		{
			command_fail(si, fault_badparams, _("Only 1-30 dice may be thrown at one time."));
			return;
		}

		if (arg_rerollflag != NULL && !strcasecmp(arg_rerollflag, "-reroll") && parv[ii + 1] != NULL)
			reroll = atoi(parv[ii++]);
		else
			reroll = 10;

		{
			end_p = buf;

			for (i = 0; i < dice; i++)
			{
				roll = atheme_random_uniform(10) + 1;

				end_p += snprintf(end_p, BUFSIZE - (end_p - buf), "%d  ", roll);

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

			gs_command_report(si, _("%s rolls %d dice: %s"), si->su->nick, dice, buf);

			if (rerolls > 0)
				gs_command_report(si, _("Successes: %d, Failures: %d, Botches: %d, Total: %d. You may reroll %d."),
					success, failure, botches, total, rerolls);
			else
				gs_command_report(si, _("Successes: %d, Failures: %d, Botches: %d, Total: %d."),
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
	char *arg_dice;
	char buf[BUFSIZE];
	int i, dice;

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;
	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DF");
		command_fail(si, fault_needmoreparams, _("Syntax: DF <dice>"));
		return;
	}
	arg_dice = parv[0];

	dice = atoi(arg_dice);
	*buf = '\0';

	if (dice > 30 || dice < 1)
	{
		command_fail(si, fault_badparams, _("Only 1-30 dice may be thrown at one time."));
		return;
	}

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
