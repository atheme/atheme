/*
 * Copyright (c) 2005-2006 William Pitcock <nenolod@nenolod.net> et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * Dice generator fantasy command.
 *
 * $Id: fc_dice.c 6745 2006-10-20 19:46:45Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/fc_dice", FALSE, _modinit, _moddeinit,
	"$Id: fc_dice.c 6745 2006-10-20 19:46:45Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void command_dice(sourceinfo_t *si, int parc, char *parv[]);
static void command_wod(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_dice = { "ROLL", "Rolls one or more dice.", AC_NONE, 2, command_dice };
command_t cmd_wod = { "WOD", "WOD-style dice generation.", AC_NONE, 7, command_wod };

list_t *cs_cmdtree;

void _modinit(module_t * m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	command_add(&cmd_dice, cs_cmdtree);
	command_add(&cmd_wod, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cmd_dice, cs_cmdtree);
	command_delete(&cmd_wod, cs_cmdtree);
}

static void command_dice(sourceinfo_t *si, int parc, char *parv[])
{
	char *arg = parv[1];
	int32_t dice, sides, i, roll = 1;

	/* this command is only available on channel */
	if (!si->c)
	{
		command_fail(si, fault_noprivs, "This command is only available on channel.");
		return;
	}

	if (!arg)
		return;
	sscanf(arg, "%dd%d", &dice, &sides);

	if (dice <= 0)
	{
		dice = 1;
		sscanf(arg, "d%d", &sides);
	}

	if (dice > 256)
		dice = 256;

	if (!dice || !sides)
	{
		dice = 1;
		sides = 1;
	}

	srand(time(0));

	for (i = 0; i < dice; i++)
		roll += (rand() % sides);

	msg(chansvs.nick, parv[0], "Your roll: \2%d\2", roll);
}

static void command_wod(sourceinfo_t *si, int parc, char *parv[])
{
	int ii = 1;
	char *arg_dice = parv[ii++];
	char *arg_difficulty = parv[ii++];

	int32_t dice, difficulty;
	int32_t roll, total = 0, roll_count = 0, i;
	int32_t success = 0, failure = 0, botches = 0, rerolls = 0;

	static char buf[BUFSIZE];
	char *end_p;

	/* this command is only available on channel */
	if (!si->c)
	{
		command_fail(si, fault_noprivs, "This command is only available on channel.");
		return;
	}

	srand(CURRTIME);

	if (arg_dice == NULL || arg_difficulty == NULL)
	{
		command_fail(si, fault_needmoreparams, "Syntax: !wod <dice> <difficulty>");
		return;
	}

	while (roll_count < 3 && arg_dice != NULL && arg_difficulty != NULL)
	{
		total = 0, success = 0, failure = 0, botches = 0, rerolls = 0;
		roll_count++;

		dice = atoi(arg_dice);
		difficulty = atoi(arg_difficulty);

		if (dice > 30 || dice < 1)
		{
			command_fail(si, fault_badparams, "Only 1-30 dice may be thrown at one time.");
			return;
		}
		else if (difficulty > 10 || difficulty < 1)
		{
			command_fail(si, fault_badparams, "Difficulty setting must be between 1 and 10.");
			return;
		}
		else
		{
			end_p = buf;

			for (i = 0; i < dice; i++)
			{
				roll = (rand() % 10) + 1;

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

			msg(chansvs.nick, parv[0], "%s rolls %d dice at difficulty %d: %s", si->su->nick, dice, difficulty, buf);

			if (rerolls > 0)
			{
				msg(chansvs.nick, parv[0], "Successes: %d, Failures: %d, Botches: %d, Total: %d. "
					"You may reroll %d if you have a specialty.",
					success, failure, botches, total, rerolls);
			}
			else
			{
				msg(chansvs.nick, parv[0], "Successes: %d, Failures: %d, Botches: %d, Total: %d.",
					success, failure, botches, total);
			}
		}

		/* prepare for another go. */
		arg_dice = parv[ii++];
		arg_difficulty = parv[ii++];
	}
}
