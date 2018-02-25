/* main.c - rpgserv main() routine.
 * based on elly's rpgserv for atheme-6.x --nenolod
 */

#include "atheme.h"

static void rs_cmd_help(struct sourceinfo *si, int parc, char *parv[]);

static struct command rs_help = { "HELP", N_("Displays contextual help information."),
                      AC_NONE, 2, rs_cmd_help, { .path = "help" } };

static void
rs_cmd_help(struct sourceinfo *si, int parc, char *parv[])
{
	char *command = parv[0];
	if (!command)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), si->service->nick);
		command_success_nodata(si, _("\2%s\2 allows users to search for game channels by matching on properties."), si->service->nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information on a command, type:"));
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		command_success_nodata(si, " ");
		command_help(si, si->service->commands);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
		return;
	}

	help_display(si, si->service, command, si->service->commands);
}

static void
mod_init(struct module *const restrict m)
{
	service_named_bind_command("rpgserv", &rs_help);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("rpgserv", &rs_help);
}

SIMPLE_DECLARE_MODULE_V1("rpgserv/help", MODULE_UNLOAD_CAPABILITY_OK)
