/*
 * Copyright (c) 2006 Jilles Tjoelker, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ GROUP command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/group", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_group(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_ungroup(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_fungroup(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_group = { "GROUP", N_("Adds a nickname to your account."), AC_NONE, 0, ns_cmd_group, { .path = "nickserv/group" } };
command_t ns_ungroup = { "UNGROUP", N_("Removes a nickname from your account."), AC_NONE, 1, ns_cmd_ungroup, { .path = "nickserv/ungroup" } };
command_t ns_fungroup = { "FUNGROUP", N_("Forces removal of a nickname from an account."), PRIV_USER_ADMIN, 2, ns_cmd_fungroup, { .path = "nickserv/fungroup" } };

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_group);
	service_named_bind_command("nickserv", &ns_ungroup);
	service_named_bind_command("nickserv", &ns_fungroup);
}

void _moddeinit()
{
	service_named_unbind_command("nickserv", &ns_group);
	service_named_unbind_command("nickserv", &ns_ungroup);
	service_named_unbind_command("nickserv", &ns_fungroup);
}

static void ns_cmd_group(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn;
	hook_user_req_t hdata;
	hook_user_register_check_t hdata_reg;

	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), "GROUP");
		return;
	}

	if (nicksvs.no_nick_ownership)
	{
		command_fail(si, fault_noprivs, _("Nickname ownership is disabled."));
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (MOWGLI_LIST_LENGTH(&si->smu->nicks) >= me.maxnicks && !has_priv(si, PRIV_REG_NOLIMIT))
	{
		command_fail(si, fault_noprivs, _("You have too many nicks registered already."));
		return;
	}

	mn = mynick_find(si->su->nick);
	if (mn != NULL)
	{
		if (mn->owner == si->smu)
			command_fail(si, fault_nochange, _("Nick \2%s\2 is already registered to your account."), mn->nick);
		else
			command_fail(si, fault_alreadyexists, _("Nick \2%s\2 is already registered to \2%s\2."), mn->nick, entity(mn->owner)->name);
		return;
	}

	if (IsDigit(si->su->nick[0]))
	{
		command_fail(si, fault_badparams, _("For security reasons, you can't register your UID."));
		return;
	}

	hdata_reg.si = si;
	hdata_reg.account = si->su->nick;
	hdata_reg.email = si->smu->email;
	hdata_reg.approved = 0;
	hook_call_nick_can_register(&hdata_reg);
	if (hdata_reg.approved != 0)
		return;

	logcommand(si, CMDLOG_REGISTER, "GROUP: \2%s\2 to \2%s\2", si->su->nick, entity(si->smu)->name);
	mn = mynick_add(si->smu, si->su->nick);
	mn->registered = CURRTIME;
	mn->lastseen = CURRTIME;
	command_success_nodata(si, _("Nick \2%s\2 is now registered to your account."), mn->nick);
	hdata.si = si;
	hdata.mu = si->smu;
	hdata.mn = mn;
	hook_call_nick_group(&hdata);
}

static void ns_cmd_ungroup(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn;
	const char *target;
	hook_user_req_t hdata;

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (parc >= 1)
		target = parv[0];
	else if (si->su != NULL)
		target = si->su->nick;
	else
		target = "?";

	mn = mynick_find(target);
	if (mn == NULL)
	{
		command_fail(si, fault_nosuch_target, _("Nick \2%s\2 is not registered."), target);
		return;
	}
	if (mn->owner != si->smu)
	{
		command_fail(si, fault_noprivs, _("Nick \2%s\2 is not registered to your account."), mn->nick);
		return;
	}
	if (!irccasecmp(mn->nick, entity(si->smu)->name))
	{
		command_fail(si, fault_noprivs, _("Nick \2%s\2 is your account name; you may not remove it."), mn->nick);
		return;
	}

	logcommand(si, CMDLOG_REGISTER, "UNGROUP: \2%s\2", mn->nick);
	hdata.si = si;
	hdata.mu = si->smu;
	hdata.mn = mn;
	hook_call_nick_ungroup(&hdata);
	command_success_nodata(si, _("Nick \2%s\2 has been removed from your account."), mn->nick);
	object_unref(mn);
}

static void ns_cmd_fungroup(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn, *mn2 = NULL;
	myuser_t *mu;
	hook_user_req_t hdata;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FUNGROUP");
		command_fail(si, fault_needmoreparams, _("Syntax: FUNGROUP <nickname> [newaccountname]"));
		return;
	}

	mn = mynick_find(parv[0]);
	if (mn == NULL)
	{
		command_fail(si, fault_nosuch_target, _("Nick \2%s\2 is not registered."), parv[0]);
		return;
	}
	mu = mn->owner;
	if (!irccasecmp(mn->nick, entity(mu)->name))
	{
		if (MOWGLI_LIST_LENGTH(&mu->nicks) <= 1 ||
			       !module_find_published("nickserv/set_accountname"))
		{
			command_fail(si, fault_noprivs, _("Nick \2%s\2 is an account name; you may not remove it."), mn->nick);
			return;
		}
		if (is_conf_soper(mu))
		{
			command_fail(si, fault_noprivs, _("You may not modify \2%s\2's account name because their operclass is defined in the configuration file."),
					entity(mu)->name);
			return;
		}
		if (parc < 2)
		{
			command_fail(si, fault_needmoreparams, _("Please specify a new account name for \2%s\2."), entity(mu)->name);
			command_fail(si, fault_needmoreparams, _("Syntax: FUNGROUP <nickname> <newaccountname>"));
			return;
		}
		mn2 = mynick_find(parv[1]);
		if (mn2 == NULL)
		{
			command_fail(si, fault_nosuch_target, _("Nick \2%s\2 is not registered."), parv[1]);
			return;
		}
		if (mn2 == mn)
		{
			command_fail(si, fault_noprivs, _("The new account name must be different from the nick to be ungrouped."));
			return;
		}
		if (mn2->owner != mu)
		{
			command_fail(si, fault_noprivs, _("Nick \2%s\2 is not registered to \2%s\2."), mn2->nick, entity(mu)->name);
			return;
		}
	}
	else if (parc > 1)
	{
		command_fail(si, fault_badparams, _("Nick \2%s\2 is not an account name so no new account name is needed."), mn->nick);
		return;
	}

	if (mn2 != NULL)
	{
		logcommand(si, CMDLOG_ADMIN | LG_REGISTER, "FUNGROUP: \2%s\2 from \2%s\2 (new account name: \2%s\2)", mn->nick, entity(mu)->name, mn2->nick);
		wallops("%s dropped the nick \2%s\2 from %s, changing account name to \2%s\2",
				get_oper_name(si), mn->nick, entity(mu)->name,
				mn2->nick);
		myuser_rename(mu, mn2->nick);
	}
	else
	{
		logcommand(si, CMDLOG_ADMIN | LG_REGISTER, "FUNGROUP: \2%s\2 from \2%s\2", mn->nick, entity(mu)->name);
		wallops("%s dropped the nick \2%s\2 from %s",
				get_oper_name(si), mn->nick, entity(mu)->name);
	}
	hdata.si = si;
	hdata.mu = mu;
	hdata.mn = mn;
	hook_call_nick_ungroup(&hdata);
	if (mn2 != NULL)
		command_success_nodata(si, _("Nick \2%s\2 has been removed from account \2%s\2, name changed to \2%s\2."), mn->nick, entity(mu)->name, mn2->nick);
	else
		command_success_nodata(si, _("Nick \2%s\2 has been removed from account \2%s\2."), mn->nick, entity(mu)->name);
	object_unref(mn);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
