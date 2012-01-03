/*
 * Copyright (c) 2011 Alexandria Wolcott
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the body of StatServ.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
("statserv/main", false, _modinit, _moddeinit,
 PACKAGE_STRING, "Alexandria Wolcott <alyx@sporksmoo.net>");

service_t *statsvs;

static void ss_cmd_help(sourceinfo_t * si, int parc, char *parv[]);

command_t ss_help =
{ "HELP", N_("Displays contextual help information."), AC_NONE, 2, ss_cmd_help, {.path = "help"}
};

void _modinit(module_t * m)
{
    statsvs = service_add("statserv", NULL);
    service_named_bind_command("statserv", &ss_help);
}

void _moddeinit(module_unload_intent_t intent)
{
    service_named_unbind_command("statserv", &ss_help);
    if (statsvs != NULL)
        service_delete(statsvs);
}

void ss_cmd_help(sourceinfo_t * si, int parc, char *parv[])
{
    char *command = parv[0];

    if (!command)
    {
        command_success_nodata(si, _("***** \2%s Help\2 *****"), si->service->nick);
        command_success_nodata(si, _("\2%s\2 records various network statistics."),
                si->service->nick);
        command_success_nodata(si, " ");
        command_success_nodata(si, _("For more information on a command, type:"));
        command_success_nodata(si, "\2/%s%s help <command>\2",
                (ircd->uses_rcommand == false) ? "msg " : "",
                si->service->disp);
        command_success_nodata(si, " ");

        command_help(si, si->service->commands);

        command_success_nodata(si, _("***** \2End of Help\2 *****"));
        return;
    }

    help_display(si, si->service, command, si->service->commands);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
