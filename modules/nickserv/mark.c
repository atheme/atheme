/*
 * Copyright (c) 2005 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Marking for nicknames.
 */

#include "atheme.h"
#include "list_common.h"
#include "list.h"

//NickServ mark module
//Do NOT use this in combination with contrib/multimark!

static void ns_cmd_mark(struct sourceinfo *si, int parc, char *parv[]);

static struct command ns_mark = { "MARK", N_("Adds a note to a user."), PRIV_MARK, 3, ns_cmd_mark, { .path = "nickserv/mark" } };

static bool
mark_match(const struct mynick *mn, const void *arg)
{
	const char *markpattern = (const char*)arg;
	struct metadata *mdmark;

	struct myuser *mu = mn->owner;
	mdmark = metadata_find(mu, "private:mark:reason");

	if (mdmark != NULL && !match(markpattern, mdmark->value))
		return true;

	return false;
}

static bool
is_marked(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return !!metadata_find(mu, "private:mark:setter");
}

static void
ns_cmd_mark(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	char *info = parv[2];
	struct myuser *mu;
	struct myuser_name *mun;

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
			atheme_object_unref(mun);
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

static void
mod_init(struct module *const restrict m)
{
	if (module_find_published("nickserv/multimark"))
	{
		slog(LG_INFO, "Loading both multimark and mark has severe consequences for the space-time continuum. Refusing to load.");
		m->mflags |= MODTYPE_FAIL;
		return;
	}

	service_named_bind_command("nickserv", &ns_mark);

	use_nslist_main_symbols(m);

	static struct list_param mark;
	mark.opttype = OPT_STRING;
	mark.is_match = mark_match;

	static struct list_param marked;
	marked.opttype = OPT_BOOL;
	marked.is_match = is_marked;

	list_register("mark-reason", &mark);
	list_register("marked", &marked);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_mark);

	list_unregister("mark-reason");
	list_unregister("marked");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/mark", MODULE_UNLOAD_CAPABILITY_OK)
