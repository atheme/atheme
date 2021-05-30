/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService BAN/UNBAN function.
 */

#include <atheme.h>

static void
cs_cmd_ban(struct sourceinfo *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *target = parv[1];
	char *newtarget;
	struct channel *c = channel_find(channel);
	struct mychan *mc = mychan_find(channel);
	struct user *tu;

	if (!channel || !target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "BAN");
		command_fail(si, fault_needmoreparams, _("Syntax: BAN <#channel> <nickname|hostmask>"));
		return;
	}

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
		return;
	}

	if (!c)
	{
		command_fail(si, fault_nosuch_target, STR_CHANNEL_IS_EMPTY, channel);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_REMOVE))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, channel);
		return;
	}

	if ((tu = user_find_named(target)))
	{
		char hostbuf[BUFSIZE];

		hostbuf[0] = '\0';

		mowgli_strlcat(hostbuf, "*!*@", BUFSIZE);
		mowgli_strlcat(hostbuf, tu->vhost, BUFSIZE);

		modestack_mode_param(chansvs.nick, c, MTYPE_ADD, 'b', hostbuf);
		chanban_add(c, hostbuf, 'b');
		logcommand(si, CMDLOG_DO, "BAN: \2%s\2 on \2%s\2 (for user \2%s!%s@%s\2)", hostbuf, mc->name, tu->nick, tu->user, tu->vhost);
		if (si->su == NULL || !chanuser_find(mc->chan, si->su))
			command_success_nodata(si, _("Banned \2%s\2 on \2%s\2."), target, channel);
		return;
	}
	else if ((is_extban(target) && (newtarget = target)) || ((newtarget = pretty_mask(target)) && validhostmask(newtarget)))
	{
		modestack_mode_param(chansvs.nick, c, MTYPE_ADD, 'b', newtarget);
		chanban_add(c, newtarget, 'b');
		logcommand(si, CMDLOG_DO, "BAN: \2%s\2 on \2%s\2", newtarget, mc->name);
		if (si->su == NULL || !chanuser_find(mc->chan, si->su))
			command_success_nodata(si, _("Banned \2%s\2 on \2%s\2."), newtarget, channel);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, _("Invalid nickname/hostmask provided: \2%s\2"), target);
		command_fail(si, fault_badparams, _("Syntax: BAN <#channel> <nickname|hostmask>"));
		return;
	}
}

static void
cs_cmd_unban(struct sourceinfo *si, int parc, char *parv[])
{
        const char *channel = parv[0];
        const char *target = parv[1];
        struct channel *c = channel_find(channel);
	struct mychan *mc = mychan_find(channel);
	struct user *tu;
	struct chanban *cb;

	if (!channel)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "UNBAN");
		command_fail(si, fault_needmoreparams, _("Syntax: UNBAN <#channel> <nickname|hostmask>"));
		return;
	}

	if (!target)
	{
		if (si->su == NULL)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "UNBAN");
			command_fail(si, fault_needmoreparams, _("Syntax: UNBAN <#channel> <nickname|hostmask>"));
			return;
		}
		target = si->su->nick;
	}

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
		return;
	}

	if (!c)
	{
		command_fail(si, fault_nosuch_target, STR_CHANNEL_IS_EMPTY, channel);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_REMOVE) &&
			(si->su == NULL ||
			 !chanacs_source_has_flag(mc, si, CA_EXEMPT) ||
			 irccasecmp(target, si->su->nick)))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, channel);
		return;
	}

	if ((tu = user_find_named(target)))
	{
		mowgli_node_t *n, *tn;
		char hostbuf2[BUFSIZE];
		unsigned int count = 0;

		snprintf(hostbuf2, BUFSIZE, "%s!%s@%s", tu->nick, tu->user, tu->vhost);
		for (n = next_matching_ban(c, tu, 'b', c->bans.head); n != NULL; n = next_matching_ban(c, tu, 'b', tn))
		{
			tn = n->next;
			cb = n->data;

			logcommand(si, CMDLOG_DO, "UNBAN: \2%s\2 on \2%s\2 (for user \2%s\2)", cb->mask, mc->name, hostbuf2);
			modestack_mode_param(chansvs.nick, c, MTYPE_DEL, cb->type, cb->mask);
			chanban_delete(cb);
			count++;
		}
		if (count > 0)
			command_success_nodata(si, ngettext(N_("Unbanned \2%s\2 on \2%s\2 (%u ban removed)."),
			                                    N_("Unbanned \2%s\2 on \2%s\2 (%u bans removed)."),
			                                    count), target, channel, count);
		else
			command_success_nodata(si, _("No bans found matching \2%s\2 on \2%s\2."), target, channel);
		return;
	}
	else if ((cb = chanban_find(c, target, 'b')) != NULL || validhostmask(target))
	{
		if (cb)
		{
			modestack_mode_param(chansvs.nick, c, MTYPE_DEL, 'b', target);
			chanban_delete(cb);
			logcommand(si, CMDLOG_DO, "UNBAN: \2%s\2 on \2%s\2", target, mc->name);
			if (si->su == NULL || !chanuser_find(mc->chan, si->su))
				command_success_nodata(si, _("Unbanned \2%s\2 on \2%s\2."), target, channel);
		}
		else
			command_fail(si, fault_nosuch_key, _("No such ban \2%s\2 on \2%s\2."), target, channel);

		return;
	}
        else
        {
		command_fail(si, fault_badparams, _("Invalid nickname/hostmask provided: \2%s\2"), target);
		command_fail(si, fault_badparams, _("Syntax: UNBAN <#channel> [nickname|hostmask]"));
		return;
        }
}

static struct command cs_ban = {
	.name           = "BAN",
	.desc           = N_("Sets a ban on a channel."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &cs_cmd_ban,
	.help           = { .path = "cservice/ban" },
};

static struct command cs_unban = {
	.name           = "UNBAN",
	.desc           = N_("Removes a ban on a channel."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &cs_cmd_unban,
	.help           = { .path = "cservice/unban" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_CONFLICT(m, "chanserv/unban_self")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_ban);
	service_named_bind_command("chanserv", &cs_unban);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_ban);
	service_named_unbind_command("chanserv", &cs_unban);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/ban", MODULE_UNLOAD_CAPABILITY_OK)
