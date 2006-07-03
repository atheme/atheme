/*
 * Copyright (c) 2005 William Pitcock <nenolod@nenolod.net>
 * Rights to this code are documented in doc/LICENSE.
 *
 * Dice generator fantasy command.
 *
 * $Id: fc_dice.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/fc_dice", FALSE, _modinit, _moddeinit,
	"$Id: fc_dice.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void fcommand_dice(char *origin, char *channel);

fcommand_t fc_dice = { "!roll", AC_NONE, fcommand_dice };

list_t *cs_fcmdtree;

void _modinit(module_t * m)
{
	MODULE_USE_SYMBOL(cs_fcmdtree, "chanserv/main", "cs_fcmdtree");
	fcommand_add(&fc_dice, cs_fcmdtree);
}

void _moddeinit()
{
	fcommand_delete(&fc_dice, cs_fcmdtree);
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
