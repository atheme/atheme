/*
 * Copyright (c) 2011 Alexandria Wolcott
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the body of StatServ.
 */

#include "atheme.h"

static struct service *statsvs = NULL;

void
ss_cmd_help(struct sourceinfo * si, int parc, char *parv[])
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

static struct command ss_help = {
	.name           = "HELP",
	.desc           = N_("Displays contextual help information."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &ss_cmd_help,
	.help           = { .path = "help" },
};

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
    statsvs = service_add("statserv", NULL);
    service_named_bind_command("statserv", &ss_help);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
    service_named_unbind_command("statserv", &ss_help);
    if (statsvs != NULL)
        service_delete(statsvs);
}

SIMPLE_DECLARE_MODULE_V1("statserv/main", MODULE_UNLOAD_CAPABILITY_OK)
