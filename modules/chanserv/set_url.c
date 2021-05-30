/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2006-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the CService SET URL command.
 */

#include <atheme.h>

static mowgli_patricia_t **cs_set_cmdtree = NULL;

static void
cs_cmd_set_url(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	char *url = parv[1];

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

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, parv[0]);
		return;
	}

	if (!url || !strcasecmp("OFF", url) || !strcasecmp("NONE", url))
	{
		/* not in a namespace to allow more natural use of SET PROPERTY.
		 * they may be able to introduce spaces, though. c'est la vie.
		 */
		if (metadata_find(mc, "url"))
		{
			metadata_delete(mc, "url");
			logcommand(si, CMDLOG_SET, "SET:URL:NONE: \2%s\2", mc->name);
			verbose(mc, "\2%s\2 cleared the channel URL", get_source_name(si));
			command_success_nodata(si, _("The URL for \2%s\2 has been cleared."), parv[0]);
			return;
		}

		command_fail(si, fault_nochange, _("The URL for \2%s\2 was not set."), parv[0]);
		return;
	}

	// we'll overwrite any existing metadata
	metadata_add(mc, "url", url);

	logcommand(si, CMDLOG_SET, "SET:URL: \2%s\2 \2%s\2", mc->name, url);
	verbose(mc, "\2%s\2 set the channel URL to \2%s\2", get_source_name(si), url);
	command_success_nodata(si, _("The URL of \2%s\2 has been set to \2%s\2."), parv[0], url);
}

static void
send_url(struct hook_channel_joinpart *hdata)
{
	struct chanuser *cu = hdata->cu;

	if (cu == NULL || is_internal_client(cu->user))
		return;

	struct mychan *mc = mychan_find(cu->chan->name);
	if (mc == NULL)
		return;

	struct user *u = cu->user;
	// Don't (re-)send on-join messages during burst
	if (!(u->server->flags & SF_EOB))
		return;

	struct metadata *md = metadata_find(mc, "url");
	if (md)
		numeric_sts(me.me, 328, u, "%s :%s", mc->name, md->value);
}

static struct command cs_set_url = {
	.name           = "URL",
	.desc           = N_("Sets the channel URL."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_set_url,
	.help           = { .path = "cservice/set_url" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree")

	command_add(&cs_set_url, *cs_set_cmdtree);
	hook_add_channel_join(&send_url);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	hook_del_channel_join(&send_url);
	command_delete(&cs_set_url, *cs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/set_url", MODULE_UNLOAD_CAPABILITY_OK)
