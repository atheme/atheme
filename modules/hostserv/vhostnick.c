/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2009 William Pitcock <nenolod -at- nenolod.net>
 *
 * Allows setting a vhost on a nick
 */

#include <atheme.h>
#include "hostserv.h"

// VHOSTNICK <nick> [host]
static void
hs_cmd_vhostnick(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *host = parv[1];
	struct myuser *mu;
	struct user *u;
	struct metadata *md;
	char buf[BUFSIZE];
	mowgli_node_t *n;
	int found = 0;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "VHOSTNICK");
		command_fail(si, fault_needmoreparams, _("Syntax: VHOSTNICK <nick> [vhost]"));
		return;
	}

	// find the user...
	if (!(mu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
		return;
	}

	MOWGLI_ITER_FOREACH(n, mu->nicks.head)
	{
		if (!irccasecmp(((struct mynick *)(n->data))->nick, target))
		{
			snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", ((struct mynick *)(n->data))->nick);
			found++;
		}
	}

	if (!found)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a valid target."), target);
		return;
	}

	// deletion action
	if (!host)
	{
		metadata_delete(mu, buf);
		command_success_nodata(si, _("Deleted vhost for \2%s\2."), target);
		logcommand(si, CMDLOG_ADMIN, "VHOSTNICK:REMOVE: \2%s\2", target);
		u = user_find_named(target);
		if (u != NULL)
		{
			// Revert to account's vhost
			md = metadata_find(mu, "private:usercloak");
			do_sethost(u, md ? md->value : NULL);
		}
		return;
	}

	if (!check_vhost_validity(si, host))
		return;

	metadata_add(mu, buf, host);
	command_success_nodata(si, _("Assigned vhost \2%s\2 to \2%s\2."),
			host, target);
	logcommand(si, CMDLOG_ADMIN, "VHOSTNICK:ASSIGN: \2%s\2 to \2%s\2",
			host, target);
	u = user_find_named(target);
	if (u != NULL)
		do_sethost(u, host);
	return;
}

static struct command hs_vhostnick = {
	.name           = "VHOSTNICK",
	.desc           = N_("Manages per-nick virtual hosts."),
	.access         = PRIV_USER_VHOST,
	.maxparc        = 2,
	.cmd            = &hs_cmd_vhostnick,
	.help           = { .path = "hostserv/vhostnick" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "hostserv/main")

	service_named_bind_command("hostserv", &hs_vhostnick);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("hostserv", &hs_vhostnick);
}

SIMPLE_DECLARE_MODULE_V1("hostserv/vhostnick", MODULE_UNLOAD_CAPABILITY_OK)
