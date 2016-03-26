/*
 * Copyright (c) 2005-2007 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ GHOST function.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/ghost", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	VENDOR_STRING
);

static void ns_cmd_ghost(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_ghost = { "GHOST", N_("Reclaims use of a nickname."), AC_NONE, 2, ns_cmd_ghost, { .path = "nickserv/ghost" } };

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_ghost);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("nickserv", &ns_ghost);
}

void ns_cmd_ghost(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	char *target = parv[0];
	char *password = parv[1];
	user_t *target_u;
	mynick_t *mn;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GHOST");
		command_fail(si, fault_needmoreparams, _("Syntax: GHOST <target> [password]"));
		return;
	}

	if (nicksvs.no_nick_ownership)
		mn = NULL, mu = myuser_find(target);
	else
	{
		mn = mynick_find(target);
		mu = mn != NULL ? mn->owner : NULL;
	}

	target_u = user_find_named(target);
	if (!target_u)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), target);
		return;
	}
	else if (!mu && !target_u->myuser)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered nickname."), target);
		return;
	}
	else if (target_u == si->su)
	{
		command_fail(si, fault_badparams, _("You may not ghost yourself."));
		return;
	}

	/* At this point, we're now checking metadata associated with the target's account.
	 * If the target nick is unregistered, mu will be unset thus far. */
	if (target_u->myuser && !mu)
		mu = target_u->myuser;

	if (password && metadata_find(mu, "private:freeze:freezer"))
	{
		command_fail(si, fault_authfail, "You cannot ghost users as \2%s\2 because the account has been frozen.", entity(mu)->name);
		logcommand(si, CMDLOG_DO, "failed GHOST \2%s\2 (frozen)", target);
		return;
	}

	if (password && (mu->flags & MU_NOPASSWORD))
	{
		command_fail(si, fault_authfail, _("Password authentication is disabled for this account."));
		logcommand(si, CMDLOG_DO, "failed GHOST \2%s\2 (password authentication disabled)", target);
		return;
	}

	if ((target_u->myuser && target_u->myuser == si->smu) || /* they're identified under our account */
			(!nicksvs.no_nick_ownership && mn && mu == si->smu) || /* we're identified under their nick's account */
			(!nicksvs.no_nick_ownership && password && verify_password(mu, password))) /* we have the correct password */
	{
		logcommand(si, CMDLOG_DO, "GHOST: \2%s!%s@%s\2", target_u->nick, target_u->user, target_u->vhost);

		kill_user(si->service->me, target_u, "GHOST command used by %s",
				si->su != NULL && !strcmp(si->su->user, target_u->user) && !strcmp(si->su->vhost, target_u->vhost) ? si->su->nick : get_source_mask(si));

		command_success_nodata(si, _("\2%s\2 has been ghosted."), target);

		/* Update the account's last seen time.
		 * Perhaps the ghosted nick belonged to someone else, but we were identified to it?
		 * Try this first. */
		if (target_u->myuser && target_u->myuser == si->smu)
			target_u->myuser->lastlogin = CURRTIME;
		else
			mu->lastlogin = CURRTIME;

		return;
	}

	if (password)
	{
		logcommand(si, CMDLOG_DO, "failed GHOST \2%s\2 (bad password)", target);
		command_fail(si, fault_authfail, _("Invalid password for \2%s\2."), entity(mu)->name);
		bad_password(si, mu);
	}
	else
	{
		logcommand(si, CMDLOG_DO, "failed GHOST \2%s\2 (invalid login)", target);
		command_fail(si, fault_noprivs, _("You may not ghost \2%s\2."), target);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
