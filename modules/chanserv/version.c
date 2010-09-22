#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/version", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_version(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_version = { "VERSION", N_("Displays version information of the services."),
                        AC_NONE, 0, cs_cmd_version, { .path = "" } };

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_version);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_version);
}

static void cs_cmd_version(sourceinfo_t *si, int parc, char *parv[])
{
        char buf[BUFSIZE];
        snprintf(buf, BUFSIZE, "Atheme %s [%s] #%s", version, revision, generation);
        command_success_string(si, buf, "%s", buf);
        return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
