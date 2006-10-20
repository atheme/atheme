#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/version", FALSE, _modinit, _moddeinit,
	"$Id: version.c 6727 2006-10-20 18:48:53Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_version(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_version = { "VERSION", "Displays version information of the services.",
                        AC_NONE, 0, cs_cmd_version };

list_t *cs_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");

        command_add(&cs_version, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_version, cs_cmdtree);
}

static void cs_cmd_version(sourceinfo_t *si, int parc, char *parv[])
{
        char buf[BUFSIZE];
        snprintf(buf, BUFSIZE, "Atheme %s [%s] #%s", version, revision, generation);
        command_success_string(si, buf, "%s", buf);
        return;
}
