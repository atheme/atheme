/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the CService SYNC functions.
 */

#include <atheme.h>

// Imported by modules/contrib/cs_modesync
extern void do_channel_sync(struct mychan *, struct chanacs *);

static mowgli_patricia_t **cs_set_cmdtree = NULL;

static bool no_vhost_sync = false;

static void
do_chanuser_sync(struct chanuser *cu, bool take)
{
	return_if_fail(cu->chan->mychan != NULL);

	if (is_internal_client(cu->user) || is_service(cu->user))
		return;

	struct hook_chanuser_sync sync_hdata = {
		.cu = cu,
		.flags = chanacs_user_flags(cu->chan->mychan, cu->user),
		.take_prefixes = take,
	};
	hook_call_chanuser_sync(&sync_hdata);
}

void
do_channel_sync(struct mychan *mc, struct chanacs *ca)
{
	struct chanuser *cu;
	mowgli_node_t *n, *tn;

	return_if_fail(mc != NULL);
	if (mc->chan == NULL)
		return;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chan->members.head)
	{
		cu = (struct chanuser *)n->data;

		if (ca)
		{
			if (ca->entity)
			{
				const struct entity_vtable *vt = myentity_get_vtable(ca->entity);
				if (!(vt->match_user && vt->match_user(ca->entity, cu->user)))
					continue;
			}
			else
			{
				if (!mask_matches_user(ca->host, cu->user))
					continue;
			}
		}

		do_chanuser_sync(cu, true);
	}
}

// this could be a little slow, should probably have an option to disable it
static void
sync_user(struct user *u)
{
	mowgli_node_t *iter;

	return_if_fail(u != NULL);

	if (no_vhost_sync)
		return;

	MOWGLI_ITER_FOREACH(iter, u->channels.head)
	{
		struct chanuser *cu = iter->data;
		struct mychan *mc;

		mc = mychan_from(cu->chan);
		if (mc == NULL)
			continue;

		do_chanuser_sync(cu, !(mc->flags & MC_NOSYNC));
	}
}

static void
sync_myuser(struct myuser *mu)
{
	mowgli_node_t *iter;

	return_if_fail(mu != NULL);

	MOWGLI_ITER_FOREACH(iter, mu->logins.head)
	{
		sync_user(iter->data);
	}
}

static void
sync_channel_acl_change(struct hook_channel_acl_req *hookdata)
{
	struct mychan *mc;

	return_if_fail(hookdata != NULL);
	return_if_fail(hookdata->ca != NULL);

	mc = hookdata->ca->mychan;
	return_if_fail(mc != NULL);

	if (MC_NOSYNC & mc->flags)
		return;

	if (((hookdata->ca->level ^ hookdata->oldlevel) &
	    (CA_AKICK | CA_EXEMPT | CA_USEOWNER | CA_USEPROTECT | CA_AUTOOP |
	     CA_OP | CA_AUTOHALFOP | CA_HALFOP | CA_AUTOVOICE | CA_VOICE)) == 0)
		return;

	do_channel_sync(mc, hookdata->ca);
}

static void
cs_cmd_sync(struct sourceinfo *si, int parc, char *parv[])
{
	char *name = parv[0];
	struct mychan *mc;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SYNC");
		command_fail(si, fault_needmoreparams, _("Syntax: SYNC <#channel>"));
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, name);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, name);
		return;
	}

	if (!mc->chan)
	{
		command_fail(si, fault_nosuch_target, STR_CHANNEL_IS_EMPTY, name);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_RECOVER))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	verbose(mc, "\2%s\2 used SYNC.", get_source_name(si));
	logcommand(si, CMDLOG_SET, "SYNC: \2%s\2", mc->name);

	do_channel_sync(mc, NULL);

	command_success_nodata(si, _("Sync complete for \2%s\2."), mc->name);
}

static void
cs_cmd_set_nosync(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (!parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET NOSYNC");
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		if (MC_NOSYNC & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "NOSYNC", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOSYNC:ON: \2%s\2", mc->name);

		mc->flags |= MC_NOSYNC;

		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "NOSYNC", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!(MC_NOSYNC & mc->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "NOSYNC", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOSYNC:OFF: \2%s\2", mc->name);

		mc->flags &= ~MC_NOSYNC;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "NOSYNC", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET NOSYNC");
		return;
	}
}

static struct command cs_sync = {
	.name           = "SYNC",
	.desc           = N_("Forces channel statuses to flags."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &cs_cmd_sync,
	.help           = { .path = "cservice/sync" },
};

static struct command cs_set_nosync = {
	.name           = "NOSYNC",
	.desc           = N_("Disables automatic channel ACL syncing."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_set_nosync,
	.help           = { .path = "cservice/set_nosync" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree")

	service_named_bind_command("chanserv", &cs_sync);

	command_add(&cs_set_nosync, *cs_set_cmdtree);

	add_bool_conf_item("NO_VHOST_SYNC", &chansvs.me->conf_table, 0, &no_vhost_sync, false);

	hook_add_channel_acl_change(sync_channel_acl_change);
	hook_add_user_sethost(sync_user);
	hook_add_user_oper(sync_user);
	hook_add_user_deoper(sync_user);
	hook_add_user_identify(sync_user);
	hook_add_user_register(sync_myuser);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	hook_del_channel_acl_change(sync_channel_acl_change);
	hook_del_user_sethost(sync_user);
	hook_del_user_oper(sync_user);
	hook_del_user_deoper(sync_user);
	hook_del_user_identify(sync_user);
	hook_del_user_register(sync_myuser);

	service_named_unbind_command("chanserv", &cs_sync);
	command_delete(&cs_set_nosync, *cs_set_cmdtree);

	del_conf_item("NO_VHOST_SYNC", &chansvs.me->conf_table);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/sync", MODULE_UNLOAD_CAPABILITY_OK)
