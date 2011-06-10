/* main.c - rpgserv main() routine.
 * based on elly's rpgserv for atheme-6.x --nenolod
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"rpgserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void rs_cmd_enable(sourceinfo_t *si, int parc, char *parv[]);
static void rs_cmd_disable(sourceinfo_t *si, int parc, char *parv[]);
static void rs_cmd_set(sourceinfo_t *si, int parc, char *parv[]);

command_t rs_enable = { "ENABLE", N_("Enable RPGServ for a channel."),
                        PRIV_HELPER, 1, rs_cmd_enable, { .path = "rpgserv/enable" } };
command_t rs_disable = { "DISABLE", N_("Disable RPGServ for a channel."),
                         PRIV_HELPER, 1, rs_cmd_disable, { .path = "rpgserv/disable" } };
command_t rs_set = { "SET", N_("Sets RPG properties of your channel."),
                     AC_NONE, 3, rs_cmd_set, { .path = "rpgserv/set" } };

service_t *rpgserv;

static void rs_cmd_enable(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	mychan_t *mc;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ENABLE");
		command_fail(si, fault_needmoreparams, _("Syntax: ENABLE <channel>"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (metadata_find(mc, "private:rpgserv:enabled"))
	{
		command_fail(si, fault_nochange, _("\2%s\2 already has RPGServ enabled."), chan);
		return;
	}

	metadata_add(mc, "private:rpgserv:enabled", si->su->nick);
	logcommand(si, CMDLOG_ADMIN, "RPGSERV:ENABLE: \2%s\2", chan);
	command_success_nodata(si, _("RPGServ enabled for \2%s\2."), chan);
}

static void rs_cmd_disable(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	mychan_t *mc;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DISABLE");
		command_fail(si, fault_needmoreparams, _("Syntax: DISABLE <channel>"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!metadata_find(mc, "private:rpgserv:enabled"))
	{
		command_fail(si, fault_nochange, _("\2%s\2 already has RPGServ disabled."), chan);
		return;
	}

	metadata_delete(mc, "private:rpgserv:enabled");
	logcommand(si, CMDLOG_ADMIN, "RPGSERV:DISABLE: \2%s\2", chan);
	command_success_nodata(si, _("RPGServ disabled for \2%s\2."), chan);
}

static void setting_clear(sourceinfo_t *si, mychan_t *mc, char *setting)
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

static int inlist(const char *needle, const char **haystack)
{
	int i;
	for (i = 0; haystack[i]; i++)
		if (!strcasecmp(needle, haystack[i]))
			return i;
	return -1;
}

static void set_genre(sourceinfo_t *si, mychan_t *mc, char *value)
{
	static const char *genres[] = {
		"apocalypse", "anime", "anthromorph", "cyberpunk",
		"fantasy", "horror", "multigenre", "realistic",
		"scifi", "steampunk", "other", NULL
	};

	if (inlist(value, genres) < 0) {
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid genre."), value);
		return;
	}

	metadata_add(mc, "private:rpgserv:genre", value);
	command_success_nodata(si, _("Genre for \2%s\2 set to \2%s\2."), mc->name, value);
}

static void set_period(sourceinfo_t *si, mychan_t *mc, char *value)
{
	static const char *periods[] = {
		"prehistoric", "antiquity", "middleages", "earlymodern",
		"modern", "future", NULL
	};

	if (inlist(value, periods) < 0) {
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid period."), value);
		return;
	}

	metadata_add(mc, "private:rpgserv:period", value);
	command_success_nodata(si, _("Period for \2%s\2 set to \2%s\2."), mc->name, value);
}

static void set_ruleset(sourceinfo_t *si, mychan_t *mc, char *value)
{
	static const char *rulesets[] = {
		"adnd", "homebrew", "dnd3.0", "dnd3.5", "dnd4.0", "freeform",
		"other", "owod", "nwod", NULL
	};

	if (inlist(value, rulesets) < 0) {
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid ruleset."), value);
		return;
	}

	metadata_add(mc, "private:rpgserv:ruleset", value);
	command_success_nodata(si, _("Ruleset for \2%s\2 set to \2%s\2."), mc->name, value);
}

static void set_rating(sourceinfo_t *si, mychan_t *mc, char *value)
{
	static const char *ratings[] = {
		"g", "pg", "pg13", "r", "adult", NULL
	};

	if (inlist(value, ratings) < 0) {
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid rating."), value);
		return;
	}

	metadata_add(mc, "private:rpgserv:rating", value);
	command_success_nodata(si, _("Rating for \2%s\2 set to \2%s\2."), mc->name, value);
}

static void set_system(sourceinfo_t *si, mychan_t *mc, char *value)
{
	char copy[512];
	char *sp = NULL, *t = NULL;
	static const char *systems[] = {
		"charapproval", "diced", "sheeted"
	};

	strlcpy(copy, value, sizeof(copy));
	t = strtok_r(copy, " ", &sp);
	while (t) {
		if (inlist(t, systems) < 0) {
			command_fail(si, fault_badparams, _("\2%s\2 is not a valid system."), t);
			return;
		}
		t = strtok_r(NULL, " ", &sp);
	}

	metadata_add(mc, "private:rpgserv:system", value);
	command_success_nodata(si, _("system for \2%s\2 set to \2%s\2."), mc->name, value);
}

static void set_setting(sourceinfo_t *si, mychan_t *mc, char *value)
{
	metadata_add(mc, "private:rpgserv:setting", value);
	command_success_nodata(si, _("Setting for \2%s\2 set."), mc->name);
}

static void set_storyline(sourceinfo_t *si, mychan_t *mc, char *value)
{
	metadata_add(mc, "private:rpgserv:storyline", value);
	command_success_nodata(si, _("Storyline for \2%s\2 set."), mc->name);
}

static void set_summary(sourceinfo_t *si, mychan_t *mc, char *value)
{
	metadata_add(mc, "private:rpgserv:summary", value);
	command_success_nodata(si, _("Summary for \2%s\2 set."), mc->name);
}

static struct {
	char *name;
	void (*func)(sourceinfo_t *si, mychan_t *mc, char *value);
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

static void rs_cmd_set(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan;
	char *setting;
	char *value = NULL;
	mychan_t *mc;
	chanacs_t *ca;
	int i;
	char nbuf[64];

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
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
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
			break;
		}
	}

	if (!settings[i].name) {
		command_fail(si, fault_badparams, _("No such setting \2%s\2."), setting);
	}
}

void _modinit(module_t *m)
{
	rpgserv = service_add("rpgserv", NULL);
	service_bind_command(rpgserv, &rs_enable);
	service_bind_command(rpgserv, &rs_disable);
	service_bind_command(rpgserv, &rs_set);
}

void _moddeinit(module_unload_intent_t intent)
{
	if (rpgserv) {
		service_delete(rpgserv);
		rpgserv = NULL;
	}

	service_unbind_command(rpgserv, &rs_enable);
	service_unbind_command(rpgserv, &rs_disable);
	service_unbind_command(rpgserv, &rs_set);
}
