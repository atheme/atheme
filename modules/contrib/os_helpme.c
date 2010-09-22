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
                        PRIV_HELPER, 0, os_cmd_helpme, { .path = "contrib/helpme" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_helpme);
}

void _moddeinit(void)
{
	service_named_unbind_command("operserv", &os_helpme);
}

static void os_cmd_helpme(sourceinfo_t *si, int parc, char *parv[])
{
	service_t *svs;

	svs = service_find("operserv");

	sts(":%s MODE %s :+h", svs->nick, si->su->nick);
	command_success_nodata(si, _("You are now a network helper."));
}
