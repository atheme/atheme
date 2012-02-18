/* search.c - rpgserv SEARCH command
 */

#include "atheme.h"
#include "prettyprint.h"

DECLARE_MODULE_V1
(
	"rpgserv/search", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void rs_cmd_search(sourceinfo_t *si, int parc, char *parv[]);

command_t rs_search = { "SEARCH", N_("Search for games based on specific criteria."),
                      AC_NONE, 20, rs_cmd_search, { .path = "rpgserv/search" } };

static void rs_cmd_search(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_patricia_iteration_state_t state;
	mychan_t *mc;
	unsigned int listed = 0;

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		unsigned int i, j;
		metadata_t *md;

		static const char *mdkeys[] = {
			"private:rpgserv:genre", "private:rpgserv:period", "private:rpgserv:ruleset",
			"private:rpgserv:rating", "private:rpgserv:system"
		};

		if (!mc->chan)
			continue;
		if (CMODE_SEC & mc->chan->modes || CMODE_PRIV & mc->chan->modes)
			continue;

		if (!metadata_find(mc, "private:rpgserv:enabled"))
			continue;

		for (i = 0; i < ARRAY_SIZE(mdkeys); i++)
		{
			md = metadata_find(mc, mdkeys[i]);
			if (md == NULL)
				continue;

			for (j = 0; j < (unsigned int) parc; j++)
			{
				if (strcasestr(md->value, parv[j]) != NULL)
					goto __matched;
			}
		}

		continue;

__matched:				
		listed++;

		command_success_nodata(si, _("Channel \2%s\2:"), mc->name);
		md = metadata_find(mc, "private:rpgserv:genre");
		command_success_nodata(si, _("Genre: %s"), rs_prettyprint_keywords(md, genre_keys, genre_names, ARRAY_SIZE(genre_keys)));
		md = metadata_find(mc, "private:rpgserv:period");
		command_success_nodata(si, _("Period: %s"), rs_prettyprint_keywords(md, period_keys, period_names, ARRAY_SIZE(period_keys)));
		md = metadata_find(mc, "private:rpgserv:ruleset");
		command_success_nodata(si, _("Ruleset: %s"), rs_prettyprint_keywords(md, ruleset_keys, ruleset_names, ARRAY_SIZE(ruleset_keys)));
		md = metadata_find(mc, "private:rpgserv:rating");
		command_success_nodata(si, _("Rating: %s"), rs_prettyprint_keywords(md, rating_keys, rating_names, ARRAY_SIZE(rating_keys)));
		md = metadata_find(mc, "private:rpgserv:system");
		command_success_nodata(si, _("System: %s"), rs_prettyprint_keywords(md, system_keys, system_names, ARRAY_SIZE(system_keys)));
	}

	command_success_nodata(si, _("\2%d\2 channels met your criteria."), listed);
	command_success_nodata(si, _("For more information on a specific channel, use \2/msg %s INFO <channel>\2."), si->service->disp);

	logcommand(si, CMDLOG_GET, "RPGSERV:SEARCH");
}

void _modinit(module_t *m)
{
	service_named_bind_command("rpgserv", &rs_search);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("rpgserv", &rs_search);
}
