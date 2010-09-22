#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/ping", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_ping(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_ping = { "PING", "Verifies network connectivity by responding with pong.",
			AC_NONE, 0, cs_cmd_ping, { .path = "contrib/cs_ping" } };

void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_ping);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_ping);
}

static void cs_cmd_ping(sourceinfo_t *si, int parc, char *parv[])
{
	command_success_nodata(si, "Pong!");
	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
