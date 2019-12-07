/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 William Pitcock, et al.
 *
 * This file contains code for the CService QUIET/UNQUIET function.
 */

#include <atheme.h>
#include "chanserv.h"

// Imported by modules/chanserv/antiflood
extern struct chanban *place_quietmask(struct channel *, int, const char *);

// Notify at most this many users in private notices, otherwise channel
#define MAX_SINGLE_NOTIFY 3

enum devoice_result
{
	DEVOICE_FAILED,
	DEVOICE_NO_ACTION,
	DEVOICE_DONE
};

static void
make_extbanmask(char *buf, size_t buflen, const char *mask)
{
	return_if_fail(buf != NULL);
	return_if_fail(mask != NULL);

	switch (ircd->type)
	{
	case PROTOCOL_INSPIRCD:
		mowgli_strlcpy(buf, "m:", buflen);
		break;
	case PROTOCOL_UNREAL:
		mowgli_strlcpy(buf, "~q:", buflen);
		break;
	default:
		*buf = '\0';
		break;
	}

	mowgli_strlcat(buf, mask, buflen);
}

static char
get_quiet_ban_char(void)
{
	return (ircd->type == PROTOCOL_UNREAL ||
			ircd->type == PROTOCOL_INSPIRCD) ? 'b' : 'q';
}

static char *
strip_extban(char *mask)
{
	if (ircd->type == PROTOCOL_INSPIRCD)
		return sstrdup(&mask[2]);
	if (ircd->type == PROTOCOL_UNREAL)
		return sstrdup(&mask[3]);
	return sstrdup(mask);
}

struct chanban *
place_quietmask(struct channel *c, int dir, const char *hostbuf)
{
	char rhostbuf[BUFSIZE];
	struct chanban *cb = NULL;
	char banlike_char = get_quiet_ban_char();

	make_extbanmask(rhostbuf, sizeof rhostbuf, hostbuf);
	modestack_mode_param(chansvs.nick, c, MTYPE_ADD, banlike_char,
			rhostbuf);
	cb = chanban_add(c, rhostbuf, banlike_char);

	return cb;
}

static void
make_extban(char *buf, size_t size, struct user *tu)
{
	return_if_fail(buf != NULL);
	return_if_fail(tu != NULL);

	switch (ircd->type)
	{
	case PROTOCOL_INSPIRCD:
		mowgli_strlcpy(buf, "m:", size);
		break;
	case PROTOCOL_UNREAL:
		mowgli_strlcpy(buf, "~q:", size);
		break;
	default:
		*buf = '\0';
		break;
	}

	mowgli_strlcat(buf, tu->nick, size);
	mowgli_strlcat(buf, "!", size);
	mowgli_strlcat(buf, tu->user, size);
	mowgli_strlcat(buf, "@", size);
	mowgli_strlcat(buf, tu->vhost, size);
}

static enum devoice_result
devoice_user(struct sourceinfo *si, struct mychan *mc, struct channel *c, struct user *tu)
{
	struct chanuser *cu;
	unsigned int flag;
	char buf[3];
	enum devoice_result result = DEVOICE_NO_ACTION;

	cu = chanuser_find(c, tu);
	if (cu == NULL)
		return DEVOICE_NO_ACTION;
	if (cu->modes & CSTATUS_OP)
		flag = CA_OP;
	else if (cu->modes & CSTATUS_VOICE)
		flag = CA_VOICE;
	else
		flag = 0;
	if (flag != 0 && !chanacs_source_has_flag(mc, si, flag))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return DEVOICE_FAILED;
	}

	if (cu->modes & CSTATUS_OWNER || cu->modes & CSTATUS_PROTECT)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is protected from quiets; you cannot quiet them."), tu->nick);
		return DEVOICE_FAILED;
	}

	buf[0] = '-';
	buf[2] = '\0';
	if (cu->modes & CSTATUS_OP)
	{
		channel_mode_va(chansvs.me->me, c, 2, "-o", tu->nick);
		result = DEVOICE_DONE;
	}
	if (cu->modes & CSTATUS_VOICE)
	{
		channel_mode_va(chansvs.me->me, c, 2, "-v", tu->nick);
		result = DEVOICE_DONE;
	}
	if (ircd->uses_owner && (cu->modes & ircd->owner_mode))
	{
		buf[1] = ircd->owner_mchar[1];
		channel_mode_va(chansvs.me->me, c, 2, buf, tu->nick);
		result = DEVOICE_DONE;
	}
	if (ircd->uses_protect && (cu->modes & ircd->protect_mode))
	{
		buf[1] = ircd->protect_mchar[1];
		channel_mode_va(chansvs.me->me, c, 2, buf, tu->nick);
		result = DEVOICE_DONE;
	}
	if (ircd->uses_halfops && (cu->modes & ircd->halfops_mode))
	{
		buf[1] = ircd->halfops_mchar[1];
		channel_mode_va(chansvs.me->me, c, 2, buf, tu->nick);
		result = DEVOICE_DONE;
	}

	return result;
}

static void
notify_one_victim(struct sourceinfo *si, struct channel *c, struct user *u, int dir)
{
	return_if_fail(dir == MTYPE_ADD || dir == MTYPE_DEL);

	// fantasy command, they can see it
	if (si->c != NULL)
		return;

	// self
	if (si->su == u)
		return;

	if (dir == MTYPE_ADD)
		change_notify(chansvs.nick, u,
				"You have been quieted on %s by %s",
				c->name, get_source_name(si));
	else if (dir == MTYPE_DEL)
		change_notify(chansvs.nick, u,
				"You have been unquieted on %s by %s",
				c->name, get_source_name(si));
}

static void
notify_victims(struct sourceinfo *si, struct channel *c, struct chanban *cb, int dir)
{
	mowgli_node_t *n;
	struct chanuser *cu;
	struct chanban tmpban;
	mowgli_list_t ban_l = { NULL, NULL, 0 };
	mowgli_node_t ban_n;
	struct user *to_notify[MAX_SINGLE_NOTIFY];
	unsigned int to_notify_count = 0, i;
	char banlike_char = get_quiet_ban_char();

	return_if_fail(dir == MTYPE_ADD || dir == MTYPE_DEL);

	if (cb == NULL)
		return;

	// fantasy command, they can see it
	if (si->c != NULL)
		return;

	/* some ircds use an action extban for mute
	 * strip it from those who do so we can reliably match users */
	memcpy(&tmpban, cb, sizeof(struct chanban));
	tmpban.mask = strip_extban(cb->mask);

	// only check the newly added/removed quiet
	mowgli_node_add(&tmpban, &ban_n, &ban_l);

	MOWGLI_ITER_FOREACH(n, c->members.head)
	{
		cu = n->data;
		if (cu->modes & (CSTATUS_OP | CSTATUS_VOICE))
			continue;
		if (is_internal_client(cu->user))
			continue;
		if (cu->user == si->su)
			continue;
		if (next_matching_ban(c, cu->user, banlike_char, &ban_n))
		{
			to_notify[to_notify_count++] = cu->user;
			if (to_notify_count >= MAX_SINGLE_NOTIFY)
				break;
		}
	}

	if (to_notify_count >= MAX_SINGLE_NOTIFY)
	{
		if (dir == MTYPE_ADD)
			notice(chansvs.nick, c->name,
					"\2%s\2 quieted \2%s\2",
					get_source_name(si), tmpban.mask);
		else if (dir == MTYPE_DEL)
			notice(chansvs.nick, c->name,
					"\2%s\2 unquieted \2%s\2",
					get_source_name(si), tmpban.mask);
	}
	else
		for (i = 0; i < to_notify_count; i++)
			notify_one_victim(si, c, to_notify[i], dir);

	sfree(tmpban.mask);
}

static void
cs_cmd_quiet(struct sourceinfo *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *target = parv[1];
	char *newtarget;
	struct channel *c = channel_find(channel);
	struct mychan *mc = mychan_find(channel);
	struct user *tu;
	struct chanban *cb;
	unsigned int n;
	char *targetlist;
	char *strtokctx = NULL;
	enum devoice_result devoice_result;

	if (!channel || !target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "QUIET");
		command_fail(si, fault_needmoreparams, _("Syntax: QUIET <#channel> <nickname|hostmask> [...]"));
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

	targetlist = sstrdup(target);
	target = strtok_r(targetlist, " ", &strtokctx);
	do
	{
		if ((tu = user_find_named(target)))
		{
			char hostbuf[BUFSIZE];

			devoice_result = devoice_user(si, mc, c, tu);
			if (devoice_result == DEVOICE_FAILED)
				continue;

			hostbuf[0] = '\0';

			mowgli_strlcat(hostbuf, "*!*@", BUFSIZE);
			mowgli_strlcat(hostbuf, tu->vhost, BUFSIZE);

			cb = place_quietmask(c, MTYPE_ADD, hostbuf);
			n = remove_ban_exceptions(si->service->me, c, tu);
			if (n > 0)
				command_success_nodata(si, ngettext(N_("To ensure the quiet takes effect, %u ban exception matching \2%s\2 has been removed from \2%s\2."),
				                                    N_("To ensure the quiet takes effect, %u ban exceptions matching \2%s\2 have been removed from \2%s\2."),
				                                    n), n, tu->nick, c->name);

			// Notify if we did anything.
			if (cb != NULL)
				notify_victims(si, c, cb, MTYPE_ADD);
			else if (devoice_result == DEVOICE_DONE || n > 0)
				notify_one_victim(si, c, tu, MTYPE_ADD);
			logcommand(si, CMDLOG_DO, "QUIET: \2%s\2 on \2%s\2 (for user \2%s!%s@%s\2)", hostbuf, mc->name, tu->nick, tu->user, tu->vhost);
			if (si->su == NULL || !chanuser_find(mc->chan, si->su))
				command_success_nodata(si, _("Quieted \2%s\2 on \2%s\2."), target, channel);
			continue;
		}
		else if ((is_extban(target) && (newtarget = target)) || ((newtarget = pretty_mask(target)) && validhostmask(newtarget)))
		{
			cb = place_quietmask(c, MTYPE_ADD, newtarget);
			notify_victims(si, c, cb, MTYPE_ADD);
			logcommand(si, CMDLOG_DO, "QUIET: \2%s\2 on \2%s\2", newtarget, mc->name);
			if (si->su == NULL || !chanuser_find(mc->chan, si->su))
				command_success_nodata(si, _("Quieted \2%s\2 on \2%s\2."), newtarget, channel);
			continue;
		}
		else
		{
			command_fail(si, fault_badparams, _("Invalid nickname/hostmask provided: \2%s\2"), target);
			command_fail(si, fault_badparams, _("Syntax: QUIET <#channel> <nickname|hostmask> [,,,]"));
			continue;
		}
	} while ((target = strtok_r(NULL, " ", &strtokctx)) != NULL);
	sfree(targetlist);
}

static void
cs_cmd_unquiet(struct sourceinfo *si, int parc, char *parv[])
{
        const char *channel = parv[0];
        const char *target = parv[1];
	char banlike_char = get_quiet_ban_char();
        struct channel *c = channel_find(channel);
	struct mychan *mc = mychan_find(channel);
	struct user *tu;
	struct chanban *cb;
	char *targetlist;
	char *strtokctx;
	char target_extban[BUFSIZE];

	if (!channel)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "UNQUIET");
		command_fail(si, fault_needmoreparams, _("Syntax: UNQUIET <#channel> <nickname|hostmask> [...]"));
		return;
	}

	if (!target)
	{
		if (si->su == NULL)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "UNQUIET");
			command_fail(si, fault_needmoreparams, _("Syntax: UNQUIET <#channel> <nickname|hostmask> [...]"));
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

	targetlist = sstrdup(target);
	target = strtok_r(targetlist, " ", &strtokctx);
	do
	{
		if ((tu = user_find_named(target)))
		{
			mowgli_node_t *n, *tn;
			char hostbuf2[BUFSIZE];
			unsigned int count = 0;

			make_extban(hostbuf2, sizeof hostbuf2, tu);
			for (n = next_matching_ban(c, tu, banlike_char, c->bans.head); n != NULL; n = next_matching_ban(c, tu, banlike_char, tn))
			{
				tn = n->next;
				cb = n->data;

				logcommand(si, CMDLOG_DO, "UNQUIET: \2%s\2 on \2%s\2 (for user \2%s\2)", cb->mask, mc->name, hostbuf2);
				modestack_mode_param(chansvs.nick, c, MTYPE_DEL, cb->type, cb->mask);
				chanban_delete(cb);
				count++;
			}
			if (count > 0)
			{
				// one notification only
				if (chanuser_find(c, tu))
					notify_one_victim(si, c, tu, MTYPE_DEL);
				command_success_nodata(si, ngettext(N_("Unquieted \2%s\2 on \2%s\2 (%u quiet removed)."),
				                                    N_("Unquieted \2%s\2 on \2%s\2 (%u quiets removed)."),
				                                    count), target, channel, count);
			}
			else
				command_success_nodata(si, _("No quiets found matching \2%s\2 on \2%s\2."), target, channel);

			continue;
		}

		(void) make_extbanmask(target_extban, sizeof target_extban, target);

		if ((cb = chanban_find(c, target_extban, banlike_char)) != NULL || validhostmask(target))
		{
			if (cb != NULL)
			{
				modestack_mode_param(chansvs.nick, c, MTYPE_DEL, banlike_char, cb->mask);
				notify_victims(si, c, cb, MTYPE_DEL);
				chanban_delete(cb);
				logcommand(si, CMDLOG_DO, "UNQUIET: \2%s\2 on \2%s\2", target_extban, mc->name);
				if (si->su == NULL || !chanuser_find(mc->chan, si->su))
					command_success_nodata(si, _("Unquieted \2%s\2 on \2%s\2."), target_extban, channel);
			}
			else
				command_fail(si, fault_nosuch_key, _("No such quiet \2%s\2 on \2%s\2."), target_extban, channel);

			continue;
		}
		else
		{
			command_fail(si, fault_badparams, _("Invalid nickname/hostmask provided: \2%s\2"), target);
			command_fail(si, fault_badparams, _("Syntax: UNQUIET <#channel> [nickname|hostmask] [...]"));
			continue;
		}

	} while ((target = strtok_r(NULL, " ", &strtokctx)) != NULL);

	sfree(targetlist);
}

static struct command cs_quiet = {
	.name           = "QUIET",
	.desc           = N_("Sets a quiet on a channel."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &cs_cmd_quiet,
	.help           = { .path = "cservice/quiet" },
};

static struct command cs_unquiet = {
	.name           = "UNQUIET",
	.desc           = N_("Removes a quiet on a channel."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &cs_cmd_unquiet,
	.help           = { .path = "cservice/unquiet" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_quiet);
	service_named_bind_command("chanserv", &cs_unquiet);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_quiet);
	service_named_unbind_command("chanserv", &cs_unquiet);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/quiet", MODULE_UNLOAD_CAPABILITY_OK)
