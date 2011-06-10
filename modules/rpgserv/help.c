/* main.c - rpgserv main() routine.
 * based on elly's rpgserv for atheme-6.x --nenolod
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"rpgserv/help", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void rs_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t rs_help = { "HELP", N_("Displays contextual help information."),
                      AC_NONE, 2, rs_cmd_help, { .path = "help" } };

static void rs_cmd_help(sourceinfo_t *si, int parc, char *parv[])
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

void _modinit(module_t *m)
{
	service_named_bind_command("rpgserv", &rs_help);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("rpgserv", &rs_help);
}
