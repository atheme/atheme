/*
 * Copyright (c) 2005 William Pitcock <nenolod@nenolod.net>
 * Rights to this code are documented in doc/LICENSE.
 *
 * Dice generator fantasy command.
 *
 * $Id: fc_dice.c 6027 2006-08-13 18:12:43Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/fc_dice", FALSE, _modinit, _moddeinit,
	"$Id: fc_dice.c 6027 2006-08-13 18:12:43Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void fcommand_dice(char *origin, char *channel);
static void fcommand_wod(char *origin, char *channel);

fcommand_t fc_dice = { "!roll", AC_NONE, fcommand_dice };
fcommand_t fc_wod = { "!wod", AC_NONE, fcommand_wod };

list_t *cs_fcmdtree;

void _modinit(module_t * m)
{
	MODULE_USE_SYMBOL(cs_fcmdtree, "chanserv/main", "cs_fcmdtree");
	fcommand_add(&fc_dice, cs_fcmdtree);
	fcommand_add(&fc_wod, cs_fcmdtree);
}

void _moddeinit()
{
	fcommand_delete(&fc_dice, cs_fcmdtree);
	fcommand_delete(&fc_wod, cs_fcmdtree);
}

static void fcommand_dice(char *origin, char *channel)
{
	char *arg = strtok(NULL, " ");
	int32_t dice, sides, i, roll = 1;

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

	msg(chansvs.nick, channel, "Your roll: \2%d\2", roll);
}

static void fcommand_wod(char *origin, char *channel)
{
	char *arg_dice = strtok(NULL, " ");
	char *arg_difficulty = strtok(NULL, " ");

	int32_t dice, difficulty;
	int32_t roll, total = 0, roll_count = 0, i;
	int32_t success = 0, failure = 0, botches = 0, rerolls = 0;

	static char buf[BUFSIZE];
	char *end_p;

	srand(CURRTIME);

	if (arg_dice == NULL || arg_difficulty == NULL)
	{
		notice(chansvs.nick, origin, "Syntax: !wod <dice> <difficulty>");
		return;
	}

	while (roll_count < 3 && arg_dice != NULL && arg_difficulty != NULL)
	{
		total = 0, success = 0, failure = 0, botches = 0, rerolls = 0;
		roll_count++;

		dice = atoi(arg_dice);
		difficulty = atoi(arg_difficulty);

		if (dice > 30 || dice < 1)
			notice(chansvs.nick, origin, "Only 1-30 dice may be thrown at one time.");
		else if (difficulty > 10 || difficulty < 1)
			notice(chansvs.nick, origin, "Difficulty setting must be between 1 and 10.");
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

			msg(chansvs.nick, channel, "%s rolls %d dice at difficulty %d: %s", origin, dice, difficulty, buf);

			if (rerolls > 0)
			{
				msg(chansvs.nick, channel, "Successes: %d, Failures: %d, Botches: %d, Total: %d. "
					"You may reroll %d if you have a specialty.",
					success, failure, botches, total, rerolls);
			}
			else
			{
				msg(chansvs.nick, channel, "Successes: %d, Failures: %d, Botches: %d, Total: %d.",
					success, failure, botches, total);
			}
		}

		/* prepare for another go. */
		arg_dice = strtok(NULL, " ");		
		arg_difficulty = strtok(NULL, " ");
	}
}
