/* list.c - rpgserv LIST command
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"rpgserv/list", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void rs_cmd_list(sourceinfo_t *si, int parc, char *parv[]);

command_t rs_list = { "LIST", N_("Lists games."),
                      AC_NONE, 0, rs_cmd_list, { .path = "rpgserv/list" } };

static void rs_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_patricia_iteration_state_t state;
	mychan_t *mc;
	unsigned int listed = 0;
	char *desc;

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		if (!mc->chan)
			continue;
		if (CMODE_SEC & mc->chan->modes || CMODE_PRIV & mc->chan->modes)
			continue;

		if (!metadata_find(mc, "private:rpgserv:enabled"))
			continue;
		if (!metadata_find(mc, "private:rpgserv:summary"))
			desc = _("<no summary>");
		else
			desc = metadata_find(mc, "private:rpgserv:summary")->value;
		command_success_nodata(si, "\2%s\2: %s", mc->name, desc);
		listed++;
	}
	command_success_nodata(si, _("Listed \2%d\2 channels."), listed);

	logcommand(si, CMDLOG_GET, "RPGSERV:LIST");
}

void _modinit(module_t *m)
{
	service_named_bind_command("rpgserv", &rs_list);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("rpgserv", &rs_list);
}
