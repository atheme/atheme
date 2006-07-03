#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/version", FALSE, _modinit, _moddeinit,
	"$Id: version.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_version(char *origin);
static void cs_fcmd_version(char *origin, char *chan);

command_t cs_version = { "VERSION", "Displays version information of the services.",
                        AC_NONE, cs_cmd_version };

fcommand_t fc_version = { "!version", AC_NONE, cs_fcmd_version };

list_t *cs_cmdtree;
list_t *cs_fcmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_fcmdtree, "chanserv/main", "cs_fcmdtree");

        command_add(&cs_version, cs_cmdtree);
	fcommand_add(&fc_version, cs_fcmdtree);
}

void _moddeinit()
{
	command_delete(&cs_version, cs_cmdtree);
	fcommand_delete(&fc_version, cs_fcmdtree);
}

static void cs_cmd_version(char *origin)
{
        char buf[BUFSIZE];
        snprintf(buf, BUFSIZE, "Atheme %s [%s] #%s", version, SERNO, generation);
        notice(chansvs.nick, origin, buf);                                         
        return;
}

static void cs_fcmd_version(char *origin, char *chan)
{
	cs_cmd_version(origin);
	return;
}
