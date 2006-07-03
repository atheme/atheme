#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/ping", FALSE, _modinit, _moddeinit,
	"$Id: ping.c 2527 2005-10-03 17:40:09Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_ping(char *origin);
static void cs_fcmd_ping(char *origin, char *chan);

command_t cs_ping = { "PING", "Verifies network connectivity by responding with pong.",
                        AC_NONE, cs_cmd_ping };

fcommand_t fc_ping = { "!ping", AC_NONE, cs_fcmd_ping };

list_t *cs_cmdtree;
list_t *cs_fcmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_fcmdtree, "chanserv/main", "cs_fcmdtree");

        command_add(&cs_ping, cs_cmdtree);
	fcommand_add(&fc_ping, cs_fcmdtree);
}

void _moddeinit()
{
	command_delete(&cs_ping, cs_cmdtree);
	fcommand_delete(&fc_ping, cs_fcmdtree);
}

static void cs_cmd_ping(char *origin)
{
        notice(chansvs.nick, origin, "Pong!");                                         
        return;
}

static void cs_fcmd_ping(char *origin, char *chan)
{
	cs_cmd_ping(origin);
	return;
}
