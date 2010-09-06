/* os_helpme.c - set user mode +h
 * elly+atheme@leptoquark.net
 */

#include "atheme.h"
#include "uplink.h"		/* sts() */


DECLARE_MODULE_V1
(
	"operserv/helpme", false, _modinit, _moddeinit,
	"os_helpme.c",
	"elly+atheme@leptoquark.net"
);

static void os_cmd_helpme(sourceinfo_t *si, int parc, char *parv[]);

command_t os_helpme = { "HELPME", N_("Makes you into a network helper."),
                        PRIV_HELPER, 0, os_cmd_helpme };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_helpme, os_cmdtree);
	help_addentry(os_helptree, "HELPME", "help/contrib/helpme", NULL);
}

void _moddeinit(void)
{
	command_delete(&os_helpme, os_cmdtree);
	help_delentry(os_helptree, "HELPME");
}

static void os_cmd_helpme(sourceinfo_t *si, int parc, char *parv[])
{
	service_t *svs;

	svs = service_find("operserv");

	sts(":%s MODE %s :+h", svs->nick, si->su->nick);
	command_success_nodata(si, _("You are now a network helper."));
}
