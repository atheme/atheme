#include "atheme.h"

DECLARE_MODULE_V1
(
	"helpserv/override", FALSE, _modinit, _moddeinit,
	"$Id: override.c 2527 2005-10-03 17:40:09Z kuja $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void hs_cmd_override(char *origin);

command_t hs_override = { "OVERRIDE", "Turns on/off staff override abilities.",
                        AC_NONE, hs_cmd_override };

list_t *hs_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(hs_cmdtree, "helpserv/main", "hs_cmdtree");
        command_add(&hs_override, hs_cmdtree);
}

void _moddeinit()
{
	command_delete(&hs_override, hs_cmdtree);
}

static void hs_cmd_override(char *origin)
{
	char *action = strtok(NULL, " ");
	myuser_t *mu;

	if (!action)
	{
		notice(helpsvs.nick, origin, STR_INSUFFICIENT_PARAMS, "OVERRIDE");
		notice(helpsvs.nick, origin, "Usage: OVERRIDE <ON|OFF>");
		return;
	}
	
	if (mu->flags & MU_HELPER)
	{
		if (!strcasecmp(action, "ON"))
		{
			if (metadata_find(mu, METADATA_USER, "private:helper:override"))
			{
				notice(helpsvs.nick, origin, "You already have override enabled.");
				return;
			}
			metadata_add(mu, METADATA_USER, "private:helper:override", "1");
			wallops("%s has turned override on!", origin);
			notice(helpsvs.nick, origin, "Override has been enabled on your account.");
		}
		else if (!strcasecmp(action, "OFF"))
		{
			if (!metadata_find(mu, METADATA_USER, "private:helper:override"))
			{
				notice(helpsvs.nick, origin, "You do not have override enabled.");
				return;
			}
			metadata_delete(mu, METADATA_USER, "private:helper:override");
			wallops("%s has turned override off.", origin);
			notice(helpsvs.nick, origin, "You've disabled override on your account.");
		}
	}
	else
	{
		notice(helpsvs.nick, origin, "You do not have sufficient privlidges to use this command.");
	}
        return;
}
