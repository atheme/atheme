/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows setting a vhost on/off
 *
 */

#include "atheme.h"
#include "hostserv.h"

DECLARE_MODULE_V1
(
	"hostserv/onoff", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void hs_cmd_on(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_off(sourceinfo_t *si, int parc, char *parv[]);

command_t hs_on = { "ON", N_("Activates your assigned vhost."), AC_NONE, 1, hs_cmd_on, { .path = "hostserv/on" } };
command_t hs_off = { "OFF", N_("Deactivates your assigned vhost."), AC_NONE, 1, hs_cmd_off, { .path = "hostserv/off" } };

void _modinit(module_t *m)
{
	service_named_bind_command("hostserv", &hs_on);
	service_named_bind_command("hostserv", &hs_off);
}

void _moddeinit(void)
{
	service_named_unbind_command("hostserv", &hs_on);
	service_named_unbind_command("hostserv", &hs_off);
}


static void hs_cmd_on(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn = NULL;
	metadata_t *md;
	char buf[BUFSIZE];

	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), "ON");
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	mn = mynick_find(si->su->nick);
	if (mn == NULL)
	{
		command_fail(si, fault_nosuch_target, _("Nick \2%s\2 is not registered."), si->su->nick);
		return;
	}
	if (mn->owner != si->smu && !myuser_access_verify(si->su, mn->owner))
	{
		command_fail(si, fault_noprivs, _("You are not recognized as \2%s\2."), entity(mn->owner)->name);
		return;
	}
	snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", mn->nick);
	md = metadata_find(si->smu, buf);
	if (md == NULL)
		md = metadata_find(si->smu, "private:usercloak");
	if (md == NULL)
	{
		command_success_nodata(si, _("Please contact an Operator to get a vhost assigned to this nick."));
		return;
	}
	do_sethost(si->su, md->value);
	command_success_nodata(si, _("Your vhost of \2%s\2 is now activated."), md->value);
}

static void hs_cmd_off(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn = NULL;
	metadata_t *md;
	char buf[BUFSIZE];

	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), "OFF");
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	mn = mynick_find(si->su->nick);
	if (mn == NULL)
	{
		command_fail(si, fault_nosuch_target, _("Nick \2%s\2 is not registered."), si->su->nick);
		return;
	}
	if (mn->owner != si->smu && !myuser_access_verify(si->su, mn->owner))
	{
		command_fail(si, fault_noprivs, _("You are not recognized as \2%s\2."), entity(mn->owner)->name);
		return;
	}
	snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", mn->nick);
	md = metadata_find(si->smu, buf);
	if (md == NULL)
		md = metadata_find(si->smu, "private:usercloak");
	if (md == NULL)
	{
		command_success_nodata(si, _("Please contact an Operator to get a vhost assigned to this nick."));
		return;
	}
	do_sethost(si->su, NULL);
	command_success_nodata(si, _("Your vhost of \2%s\2 is now deactivated."), md->value);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
