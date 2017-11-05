#include "atheme.h"
#include "serno.h"

static void cs_cmd_version(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_version = { "VERSION", N_("Displays version information of the services."),
                        AC_NONE, 0, cs_cmd_version, { .path = "" } };

static void
mod_init(module_t *const restrict m)
{
        service_named_bind_command("chanserv", &cs_version);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_version);
}

static void cs_cmd_version(sourceinfo_t *si, int parc, char *parv[])
{
        char buf[BUFSIZE];
        snprintf(buf, BUFSIZE, "%s [%s]", PACKAGE_STRING, SERNO);
        command_success_string(si, buf, "%s", buf);
        return;
}

SIMPLE_DECLARE_MODULE_V1("chanserv/version", MODULE_UNLOAD_CAPABILITY_OK)
