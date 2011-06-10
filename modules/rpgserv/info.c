/* main.c - rpgserv main() routine.
 * based on elly's rpgserv for atheme-6.x --nenolod
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"rpgserv/info", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void rs_cmd_info(sourceinfo_t *si, int parc, char *parv[]);

command_t rs_info = { "INFO", N_("Displays info for a particular game."),
                      AC_NONE, 1, rs_cmd_info, { .path = "rpgserv/info" } };

static void rs_cmd_info(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	metadata_t *md;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INFO");
		command_fail(si, fault_needmoreparams, _("Syntax: INFO <channel>"));
		return;
	}

	mc = mychan_find(parv[0]);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!metadata_find(mc, "private:rpgserv:enabled"))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 does not have RPGServ enabled."), parv[0]);
		return;
	}

	command_success_nodata(si, _("Channel \2%s\2:"), parv[0]);
	md = metadata_find(mc, "private:rpgserv:genre");
	command_success_nodata(si, _("Genre: %s"), md ? md->value : "<none>");
	md = metadata_find(mc, "private:rpgserv:period");
	command_success_nodata(si, _("Period: %s"), md ? md->value : "<none>");
	md = metadata_find(mc, "private:rpgserv:ruleset");
	command_success_nodata(si, _("Ruleset: %s"), md ? md->value : "<none>");
	md = metadata_find(mc, "private:rpgserv:rating");
	command_success_nodata(si, _("Rating: %s"), md ? md->value : "<none>");
	md = metadata_find(mc, "private:rpgserv:system");
	command_success_nodata(si, _("System: %s"), md ? md->value : "<none>");
	md = metadata_find(mc, "private:rpgserv:setting");
	command_success_nodata(si, _("Setting: %s"), md ? md->value : "<none>");
	md = metadata_find(mc, "private:rpgserv:storyline");
	command_success_nodata(si, _("Storyline: %s"), md ? md->value : "<none>");
	md = metadata_find(mc, "private:rpgserv:summary");
	command_success_nodata(si, _("Summary: %s"), md ? md->value : "<none>");	
}

void _modinit(module_t *m)
{
	service_named_bind_command("rpgserv", &rs_info);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("rpgserv", &rs_info);
}
