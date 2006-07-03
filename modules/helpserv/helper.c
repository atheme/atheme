#include "atheme.h"

DECLARE_MODULE_V1
(
	"helpserv/helper", FALSE, _modinit, _moddeinit,
	"$Id: helper.c 2527 2005-10-03 17:40:09Z kuja $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void hs_cmd_helper(char *origin);

command_t hs_helper = { "HELPER", "Allows SRAs to add helpers.",
                        AC_SRA, hs_cmd_helper };

list_t *hs_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(hs_cmdtree, "helpserv/main", "hs_cmdtree");

        command_add(&hs_helper, hs_cmdtree);
}

void _moddeinit()
{
	command_delete(&hs_helper, hs_cmdtree);
}

static void hs_cmd_helper(char *origin)
{
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	myuser_t *mu;

	if (!target || !action)
	{
		notice(helpsvs.nick, origin, STR_INSUFFICIENT_PARAMS, "ADDHELPER");
		notice(helpsvs.nick, origin, "Usage: HELPER <nickname> <ON|OFF>");
		return;
	}

	if (!(mu = myuser_find(target)))
	{
		notice(helpsvs.nick, origin, "\2%s\2 is not registered.", target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (mu->flags & MU_HELPER)
		{
			notice(helpsvs.nick, origin, "\2%s\2 is already a helper.", target);
			return;
		}

		mu->flags |= MU_HELPER;

		wallops("%s set the HELPER option for the nickname \2%s\2.", origin, target);
		notice(helpsvs.nick, origin, "\2%s\2 is now a helper.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!(mu->flags & MU_HELPER))
		{
			notice(helpsvs.nick, origin, "\2%s\2 is not a helper.", target);
			return;
		}

		mu->flags &= ~MU_HELPER;

		wallops("%s removed the HELPER option on the nickname \2%s\2.", origin, target);
		notice(helpsvs.nick, origin, "\2%s\2 is no longer a helper.", target);
	}
	else
	{
		notice(helpsvs.nick, origin, STR_INVALID_PARAMS, "HELPER");
		notice(helpsvs.nick, origin, "Usage: HELPER <nickname> <ON|OFF>");
	}
        return;
}
