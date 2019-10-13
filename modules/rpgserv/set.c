/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
 *
 * main.c - rpgserv main() routine.
 * based on elly's rpgserv for atheme-6.x --nenolod
 */

#include <atheme.h>
#include "prettyprint.h"

static void
setting_clear(struct sourceinfo *si, struct mychan *mc, char *setting)
{
	char nbuf[64];
	snprintf(nbuf, sizeof(nbuf), "private:rpgserv:%s", setting);
	if (!metadata_find(mc, nbuf)) {
		command_fail(si, fault_nochange, _("\2%s\2 has no %s."), mc->name, setting);
		return;
	}
	metadata_delete(mc, nbuf);
	command_success_nodata(si, _("Setting \2%s\2 cleared for \2%s\2."), setting, mc->name);
}

static int
inlist(const char *needle, const char **haystack)
{
	int i;
	for (i = 0; haystack[i]; i++)
		if (!strcasecmp(needle, haystack[i]))
			return i;
	return -1;
}

static void
set_genre(struct sourceinfo *si, struct mychan *mc, char *value)
{
	char copy[512];
	char *sp = NULL, *t = NULL;

	mowgli_strlcpy(copy, value, sizeof(copy));
	t = strtok_r(copy, " ", &sp);

	while(t) {
		if (inlist(t, genre_keys) < 0) {
			command_fail(si, fault_badparams, _("\2%s\2 is not a valid genre."), t);
			return;
		}
		t = strtok_r(NULL, " ", &sp);
	}

	metadata_add(mc, "private:rpgserv:genre", value);
	command_success_nodata(si, _("Genre for \2%s\2 set to \2%s\2."), mc->name, value);
}

static void
set_period(struct sourceinfo *si, struct mychan *mc, char *value)
{
	char copy[512];
	char *sp = NULL, *t = NULL;

	mowgli_strlcpy(copy, value, sizeof(copy));
	t = strtok_r(copy, " ", &sp);

	while(t) {
		if (inlist(t, period_keys) < 0) {
			command_fail(si, fault_badparams, _("\2%s\2 is not a valid period."), value);
			return;
		}
		t = strtok_r(NULL, " ", &sp);
	}

	metadata_add(mc, "private:rpgserv:period", value);
	command_success_nodata(si, _("Period for \2%s\2 set to \2%s\2."), mc->name, value);
}

static void
set_ruleset(struct sourceinfo *si, struct mychan *mc, char *value)
{
	if (inlist(value, ruleset_keys) < 0) {
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid ruleset."), value);
		return;
	}

	metadata_add(mc, "private:rpgserv:ruleset", value);
	command_success_nodata(si, _("Ruleset for \2%s\2 set to \2%s\2."), mc->name, value);
}

static void
set_rating(struct sourceinfo *si, struct mychan *mc, char *value)
{
	if (inlist(value, rating_keys) < 0) {
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid rating."), value);
		return;
	}

	metadata_add(mc, "private:rpgserv:rating", value);
	command_success_nodata(si, _("Rating for \2%s\2 set to \2%s\2."), mc->name, value);
}

static void
set_system(struct sourceinfo *si, struct mychan *mc, char *value)
{
	char copy[512];
	char *sp = NULL, *t = NULL;

	mowgli_strlcpy(copy, value, sizeof(copy));
	t = strtok_r(copy, " ", &sp);
	while (t) {
		if (inlist(t, system_keys) < 0) {
			command_fail(si, fault_badparams, _("\2%s\2 is not a valid system."), t);
			return;
		}
		t = strtok_r(NULL, " ", &sp);
	}

	metadata_add(mc, "private:rpgserv:system", value);
	command_success_nodata(si, _("System for \2%s\2 set to \2%s\2."), mc->name, value);
}

static void
set_setting(struct sourceinfo *si, struct mychan *mc, char *value)
{
	metadata_add(mc, "private:rpgserv:setting", value);
	command_success_nodata(si, _("Setting for \2%s\2 set."), mc->name);
}

static void
set_storyline(struct sourceinfo *si, struct mychan *mc, char *value)
{
	metadata_add(mc, "private:rpgserv:storyline", value);
	command_success_nodata(si, _("Storyline for \2%s\2 set."), mc->name);
}

static void
set_summary(struct sourceinfo *si, struct mychan *mc, char *value)
{
	metadata_add(mc, "private:rpgserv:summary", value);
	command_success_nodata(si, _("Summary for \2%s\2 set."), mc->name);
}

static void
rs_cmd_set(struct sourceinfo *si, int parc, char *parv[])
{
	static const struct {
		const char *name;
		void (*func)(struct sourceinfo *, struct mychan *, char *);
	} settings[] = {
		{ "genre", set_genre },
		{ "period", set_period },
		{ "ruleset", set_ruleset },
		{ "rating", set_rating },
		{ "system", set_system },
		{ "setting", set_setting },
		{ "storyline", set_storyline },
		{ "summary", set_summary },
		{ NULL, NULL },
	};

	char *chan;
	char *setting;
	char *value = NULL;
	struct mychan *mc;
	int i;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <channel> <property> [value...]"));
		return;
	}

	chan = parv[0];
	setting = parv[1];
	if (parc > 2)
		value = parv[2];

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (!metadata_find(mc, "private:rpgserv:enabled"))
	{
		command_fail(si, fault_noprivs, _("Channel \2%s\2 does not have RPGServ enabled."), chan);
		return;
	}

	for (i = 0; settings[i].name; i++) {
		if (!strcasecmp(settings[i].name, setting)) {
			if (value)
				settings[i].func(si, mc, value);
			else
				setting_clear(si, mc, setting);

			logcommand(si, CMDLOG_SET, "RPGSERV:SET: \2%s\2 (\2%s\2 -> \2%s\2)",
				   mc->name, setting, value ? value : "(cleared)");

			break;
		}
	}

	if (!settings[i].name) {
		command_fail(si, fault_badparams, _("No such setting \2%s\2."), setting);
	}
}

static struct command rs_set = {
	.name           = "SET",
	.desc           = N_("Sets RPG properties of your channel."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &rs_cmd_set,
	.help           = { .path = "rpgserv/set" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "rpgserv/main")

	service_named_bind_command("rpgserv", &rs_set);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("rpgserv", &rs_set);
}

SIMPLE_DECLARE_MODULE_V1("rpgserv/set", MODULE_UNLOAD_CAPABILITY_OK)
