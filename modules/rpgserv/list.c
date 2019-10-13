/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
 *
 * list.c - rpgserv LIST command
 */

#include <atheme.h>

static void
rs_cmd_list(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_patricia_iteration_state_t state;
	struct mychan *mc;
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
	command_success_nodata(si, _("Listed \2%u\2 channels."), listed);

	logcommand(si, CMDLOG_GET, "RPGSERV:LIST");
}

static struct command rs_list = {
	.name           = "LIST",
	.desc           = N_("Lists games."),
	.access         = AC_NONE,
	.maxparc        = 0,
	.cmd            = &rs_cmd_list,
	.help           = { .path = "rpgserv/list" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "rpgserv/main")

	service_named_bind_command("rpgserv", &rs_list);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("rpgserv", &rs_list);
}

SIMPLE_DECLARE_MODULE_V1("rpgserv/list", MODULE_UNLOAD_CAPABILITY_OK)
