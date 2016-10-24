/*
 * Copyright (c) 2015-2016 Atheme Development Group <http://atheme.github.io/>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows a user to drop (unset) their assigned vhost without
 * Staff intervention.
 *
 */

#include "atheme.h"
#include "hostserv.h"

DECLARE_MODULE_V1
(
	"hostserv/drop", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://atheme.github.io>"
);

static void hs_cmd_drop(sourceinfo_t *si, int parc, char *parv[]);

command_t hs_drop = { "DROP", N_("Drops your assigned vhost."), AC_AUTHENTICATED, 1, hs_cmd_drop, { .path = "hostserv/drop" } };

void _modinit(module_t *m)
{
	service_named_bind_command("hostserv", &hs_drop);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("hostserv", &hs_drop);
}


static void hs_cmd_drop(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn;
	metadata_t *md;
	char buf[BUFSIZE];

	/* This is only because we need a nick to copy from. */
	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), "DROP");
		return;
	}

	mn = mynick_find(si->su->nick);
	if (mn == NULL)
	{
		command_fail(si, fault_nosuch_target, _("Nick \2%s\2 is not registered."), si->su->nick);
		return;
	}
	if (mn->owner != si->smu)
	{
		command_fail(si, fault_noprivs, _("Nick \2%s\2 is not registered to your account."), mn->nick);
		return;
	}
	snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", mn->nick);
	md = metadata_find(si->smu, buf);
	if (md == NULL)
		md = metadata_find(si->smu, "private:usercloak");
	if (md == NULL)
	{
		command_success_nodata(si, _("There is not a vhost assigned to this nick."));
		return;
	}
	hs_sethost_all(si->smu, NULL, get_source_name(si));
	command_success_nodata(si, _("Dropped all vhosts for \2%s\2."), get_source_name(si));
	logcommand(si, CMDLOG_ADMIN, "VHOST:DROP: \2%s\2", get_source_name(si));
	do_sethost_all(si->smu, NULL); // restore user vhost from user host

}
