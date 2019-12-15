/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2006-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the CService SET ENTRYMSG command.
 */

#include <atheme.h>

#define ENTRYMSG_MD "private:entrymsg"

static mowgli_patricia_t **cs_set_cmdtree = NULL;

static void
cs_cmd_set_entrymsg(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (!parv[1] || !strcasecmp("OFF", parv[1]) || !strcasecmp("NONE", parv[1]))
	{
		/* entrymsg is private because users won't see it if they're AKICKED,
		 * if the channel is +i, or if the channel is RESTRICTED
		 */
		if (metadata_find(mc, ENTRYMSG_MD))
		{
			metadata_delete(mc, ENTRYMSG_MD);
			logcommand(si, CMDLOG_SET, "SET:ENTRYMSG:NONE: \2%s\2", mc->name);
			verbose(mc, "\2%s\2 cleared the entry message", get_source_name(si));
			command_success_nodata(si, _("The entry message for \2%s\2 has been cleared."), parv[0]);
			return;
		}

		command_fail(si, fault_nochange, _("The entry message for \2%s\2 was not set."), parv[0]);
		return;
	}

	/* we'll overwrite any existing metadata.
	 * Why is/was this even private? There are no size/content sanity checks
	 * and even users with no channel access can see it. --jdhore
	 */
	metadata_add(mc, ENTRYMSG_MD, parv[1]);

	logcommand(si, CMDLOG_SET, "SET:ENTRYMSG: \2%s\2 \2%s\2", mc->name, parv[1]);
	verbose(mc, "\2%s\2 set the entry message for the channel to \2%s\2", get_source_name(si), parv[1]);
	command_success_nodata(si, _("The entry message for \2%s\2 has been set to \2%s\2"), parv[0], parv[1]);
}

static void
send_entrymsg(struct hook_channel_joinpart *hdata)
{
	struct chanuser *cu = hdata->cu;

	if (cu == NULL || is_internal_client(cu->user))
		return;

	struct mychan *mc = mychan_find(cu->chan->name);
	if (mc == NULL)
		return;

	struct user *u = cu->user;
	// Don't (re-)send entry messages during burst
	if (!(u->server->flags & SF_EOB))
		return;

	struct metadata *md = metadata_find(mc, ENTRYMSG_MD);
	if (md != NULL && metadata_find(mc, "private:botserv:bot-assigned") == NULL)
	{
		if (!u->myuser || !(u->myuser->flags & MU_NOGREET))
			notice(chansvs.nick, u->nick, "[%s] %s", mc->name, md->value);
	}
}

static struct command cs_set_entrymsg = {
	.name           = "ENTRYMSG",
	.desc           = N_("Sets the channel entry message."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_set_entrymsg,
	.help           = { .path = "cservice/set_entrymsg" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree")

	command_add(&cs_set_entrymsg, *cs_set_cmdtree);
	hook_add_channel_join(&send_entrymsg);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	hook_del_channel_join(&send_entrymsg);
	command_delete(&cs_set_entrymsg, *cs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/set_entrymsg", MODULE_UNLOAD_CAPABILITY_OK)
