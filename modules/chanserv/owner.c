/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OWNER functions.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/owner", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_owner(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_deowner(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_owner = { "OWNER", N_("Gives channel owner flag to a user."),
                        AC_NONE, 2, cs_cmd_owner };
command_t cs_deowner = { "DEOWNER", N_("Removes channel owner flag from a user."),
                        AC_NONE, 2, cs_cmd_deowner };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	if (ircd != NULL && !ircd->uses_owner)
	{
		slog(LG_INFO, "Module %s requires owner support, refusing to load.", m->header->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        service_named_bind_command("chanserv", &cs_owner);
        service_named_bind_command("chanserv", &cs_deowner);

	help_addentry(cs_helptree, "OWNER", "help/cservice/owner", NULL);
	help_addentry(cs_helptree, "DEOWNER", "help/cservice/owner", NULL);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_owner);
	service_named_unbind_command("chanserv", &cs_deowner);

	help_delentry(cs_helptree, "OWNER");
	help_delentry(cs_helptree, "DEOWNER");
}

static void cs_cmd_owner(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;

	if (ircd->uses_owner == false)
	{
		command_fail(si, fault_noprivs, _("The IRCd software you are running does not support this feature."));
		return;
	}

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OWNER");
		command_fail(si, fault_needmoreparams, _("Syntax: OWNER <#channel> [nickname]"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_USEOWNER))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}
	
	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	/* figure out who we're going to op */
	if (!nick)
		tu = si->su;
	else
	{
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			return;
		}
	}

	if (is_internal_client(tu))
		return;

	/* SECURE check; we can skip this if sender == target, because we already verified */
	if ((si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_OP) && !chanacs_user_has_flag(mc, tu, CA_AUTOOP))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		command_fail(si, fault_noprivs, _("\2%s\2 has the SECURE option enabled, and \2%s\2 does not have appropriate access."), mc->name, tu->nick);
		return;
	}

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
		return;
	}

	modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, ircd->owner_mchar[1], CLIENT_NAME(tu));
	cu->modes |= CSTATUS_OWNER;

	if (si->c == NULL && tu != si->su)
		change_notify(chansvs.nick, tu, "You have been set as owner on %s by %s", mc->name, get_source_name(si));

	logcommand(si, CMDLOG_DO, "OWNER: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost, mc->name);
	if (!chanuser_find(mc->chan, si->su))
		command_success_nodata(si, _("\2%s\2 has been set as owner on \2%s\2."), tu->nick, mc->name);
}

static void cs_cmd_deowner(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;

	if (ircd->uses_owner == false)
	{
		command_fail(si, fault_noprivs, _("The IRCd software you are running does not support this feature."));
		return;
	}

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEOWNER");
		command_fail(si, fault_needmoreparams, _("Syntax: DEOWNER <#channel> [nickname]"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_USEOWNER))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}
	
	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	/* figure out who we're going to deop */
	if (!nick)
		tu = si->su;
	else
	{
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			return;
		}
	}

	if (is_internal_client(tu))
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
		return;
	}

	modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->owner_mchar[1], CLIENT_NAME(tu));
	cu->modes &= ~CSTATUS_OWNER;

	if (si->c == NULL && tu != si->su)
		change_notify(chansvs.nick, tu, "You have been unset as owner on %s by %s", mc->name, get_source_name(si));

	logcommand(si, CMDLOG_DO, "DEOWNER: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost, mc->name);
	if (!chanuser_find(mc->chan, si->su))
		command_success_nodata(si, _("\2%s\2 has been unset as owner on \2%s\2."), tu->nick, mc->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
