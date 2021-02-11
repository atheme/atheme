/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * Closing for channels.
 */

#include <atheme.h>

static void
close_check_join(struct hook_channel_joinpart *data)
{
	struct mychan *mc;
	struct chanuser *cu = data->cu;

	if (cu == NULL || is_internal_client(cu->user))
		return;
	mc = mychan_find(cu->chan->name);
	if (mc == NULL)
		return;

	if (metadata_find(mc, "private:close:closer"))
	{
		// don't join if we're already in there
		if (!chanuser_find(cu->chan, user_find_named(chansvs.nick)))
			join(cu->chan->name, chansvs.nick);

		// stay for a bit to stop rejoin floods
		mc->flags |= MC_INHABIT;

		// lock it down
		channel_mode_va(chansvs.me->me, cu->chan, 3, "+isbl", "*!*@*", "1");

		// clear the channel
		if (try_kick(chansvs.me->me, cu->chan, cu->user, "This channel has been closed"))
			data->cu = NULL;
	}
}

// CLOSE ON|OFF -- don't pollute the root with REOPEN
static void
cs_cmd_close(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	char *reason = parv[2];
	struct mychan *mc;
	struct channel *c;
	struct chanuser *cu;
	mowgli_node_t *n, *tn;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLOSE");
		command_fail(si, fault_needmoreparams, _("Usage: CLOSE <#channel> <ON|OFF> [reason]"));
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!reason)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLOSE");
			command_fail(si, fault_needmoreparams, _("Usage: CLOSE <#channel> ON <reason>"));
			return;
		}

		if (mc->flags & CHAN_LOG)
		{
			command_fail(si, fault_noprivs, _("\2%s\2 cannot be closed."), target);
			return;
		}

		if (metadata_find(mc, "private:close:closer"))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already closed."), target);
			return;
		}

		metadata_add(mc, "private:close:closer", get_oper_name(si));
		metadata_add(mc, "private:close:reason", reason);
		metadata_add(mc, "private:close:timestamp", number_to_string(CURRTIME));

		if ((c = channel_find(target)))
		{
			if (!chanuser_find(c, user_find_named(chansvs.nick)))
				join(target, chansvs.nick);

			// stay for a bit to stop rejoin floods
			mc->flags |= MC_INHABIT;

			// lock it down
			channel_mode_va(chansvs.me->me, c, 3, "+isbl", "*!*@*", "1");

			// clear the channel
			MOWGLI_ITER_FOREACH_SAFE(n, tn, c->members.head)
			{
				cu = (struct chanuser *)n->data;

				if (!is_internal_client(cu->user))
					try_kick(chansvs.me->me, c, cu->user, "This channel has been closed");
			}
		}

		wallops("\2%s\2 closed the channel \2%s\2 (%s).", get_oper_name(si), target, reason);
		logcommand(si, CMDLOG_ADMIN, "CLOSE:ON: \2%s\2 (reason: \2%s\2)", target, reason);
		command_success_nodata(si, _("\2%s\2 is now closed."), target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mc, "private:close:closer"))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is not closed."), target);
			return;
		}

		metadata_delete(mc, "private:close:closer");
		metadata_delete(mc, "private:close:reason");
		metadata_delete(mc, "private:close:timestamp");
		mc->flags &= ~MC_INHABIT;
		c = channel_find(target);
		if (c != NULL)
			if (chanuser_find(c, user_find_named(chansvs.nick)))
				part(c->name, chansvs.nick);
		c = channel_find(target);
		if (c != NULL)
		{
			// hmm, channel still exists, probably permanent?
			channel_mode_va(chansvs.me->me, c, 2, "-isbl", "*!*@*");
			check_modes(mc, true);
		}

		wallops("\2%s\2 reopened the channel \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "CLOSE:OFF: \2%s\2", target);
		command_success_nodata(si, _("\2%s\2 has been reopened."), target);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "CLOSE");
		command_fail(si, fault_badparams, _("Usage: CLOSE <#channel> <ON|OFF> [reason]"));
	}
}

static struct command cs_close = {
	.name           = "CLOSE",
	.desc           = N_("Closes a channel."),
	.access         = PRIV_CHAN_ADMIN,
	.maxparc        = 3,
	.cmd            = &cs_cmd_close ,
	.help           = { .path = "cservice/close" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	service_named_bind_command("chanserv", &cs_close);
	hook_add_first_channel_join(close_check_join);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_close);
	hook_del_channel_join(close_check_join);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/close", MODULE_UNLOAD_CAPABILITY_OK)
