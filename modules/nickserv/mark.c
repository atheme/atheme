/*
 * Copyright (c) 2005 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Marking for nicknames.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/mark", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_mark(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_mark = { "MARK", N_("Adds a note to a user."), PRIV_MARK, 3, ns_cmd_mark, { .path = "nickserv/mark" } };

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_mark);
}

void _moddeinit()
{
	service_named_unbind_command("nickserv", &ns_mark);
}

static void ns_cmd_mark(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	char *info = parv[2];
	myuser_t *mu;
	myuser_name_t *mun;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
		command_fail(si, fault_needmoreparams, _("Usage: MARK <target> <ON|OFF> [note]"));
		return;
	}

	if (!(mu = myuser_find_ext(target)))
	{
		mun = myuser_name_find(target);
		if (mun != NULL && !strcasecmp(action, "OFF"))
		{
			object_unref(mun);
			wallops("%s unmarked the name \2%s\2.", get_oper_name(si), target);
			logcommand(si, CMDLOG_ADMIN, "MARK:OFF: \2%s\2", target);
			command_success_nodata(si, _("\2%s\2 is now unmarked."), target);
			return;
		}
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!info)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
			command_fail(si, fault_needmoreparams, _("Usage: MARK <target> ON <note>"));
			return;
		}

		if (metadata_find(mu, "private:mark:setter"))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is already marked."), entity(mu)->name);
			return;
		}

		metadata_add(mu, "private:mark:setter", get_oper_name(si));
		metadata_add(mu, "private:mark:reason", info);
		metadata_add(mu, "private:mark:timestamp", number_to_string(time(NULL)));

		wallops("%s marked the account \2%s\2.", get_oper_name(si), entity(mu)->name);
		logcommand(si, CMDLOG_ADMIN, "MARK:ON: \2%s\2 (reason: \2%s\2)", entity(mu)->name, info);
		command_success_nodata(si, _("\2%s\2 is now marked."), entity(mu)->name);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mu, "private:mark:setter"))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is not marked."), entity(mu)->name);
			return;
		}

		metadata_delete(mu, "private:mark:setter");
		metadata_delete(mu, "private:mark:reason");
		metadata_delete(mu, "private:mark:timestamp");

		wallops("%s unmarked the account \2%s\2.", get_oper_name(si), entity(mu)->name);
		logcommand(si, CMDLOG_ADMIN, "MARK:OFF: \2%s\2", entity(mu)->name);
		command_success_nodata(si, _("\2%s\2 is now unmarked."), entity(mu)->name);
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "MARK");
		command_fail(si, fault_needmoreparams, _("Usage: MARK <target> <ON|OFF> [note]"));
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
