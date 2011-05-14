/* 
 * rpgserv - rpg finding service
 * Copyright (c) 2011 Atheme Development Group
 */

#include "atheme.h"
#include "rpgserv.h"

DECLARE_MODULE_V1("rpgserv/main", false, _modinit, _moddeinit,
		  PACKAGE_VERSION, "Atheme Development Group");

static const char *rpg_rating_names[RPG_RATING_COUNT] = {
	[RPG_RATING_G]		= "G",
	[RPG_RATING_PG]		= "PG",
	[RPG_RATING_PG13]	= "PG-13",
	[RPG_RATING_R]		= "R",
	[RPG_RATING_ADULT]	= "Adult",
};

static const char *rpg_rating_keys[RPG_RATING_COUNT] = {
	[RPG_RATING_G]		= "G",
	[RPG_RATING_PG]		= "PG",
	[RPG_RATING_PG13]	= "PG13",
	[RPG_RATING_R]		= "R",
	[RPG_RATING_ADULT]	= "ADULT",
};

static const char *rpg_ruleset_names[RPG_RULESET_COUNT] = {
	[RPG_RULESET_ADND]	= "ADnD",
	[RPG_RULESET_DND3]	= "DnD 3.0",
	[RPG_RULESET_DND35]	= "DnD 3.5",
	[RPG_RULESET_DND4]	= "DnD 4.0",
	[RPG_RULESET_FREEFORM]	= "Freeform",
	[RPG_RULESET_HOMEBREW]	= "Homebrew",
	[RPG_RULESET_NWOD]	= "New World of Darkness",
	[RPG_RULESET_OWOD]	= "Old World of Darkness",
	[RPG_RULESET_OTHER]	= "Other",
};

static const char *rpg_ruleset_keys[RPG_RULESET_COUNT] = {
	[RPG_RULESET_ADND]	= "ADND",
	[RPG_RULESET_DND3]	= "DND3.0",
	[RPG_RULESET_DND35]	= "DND3.5",
	[RPG_RULESET_DND4]	= "DND4.0",
	[RPG_RULESET_FREEFORM]	= "FREEFORM",
	[RPG_RULESET_HOMEBREW]	= "HOMEBREW",
	[RPG_RULESET_NWOD]	= "NWOD",
	[RPG_RULESET_OWOD]	= "OWOD",
	[RPG_RULESET_OTHER]	= "Other",
};

static const char *rpg_period_names[RPG_PERIOD_COUNT] = {
	[RPG_PERIOD_ANTIQUITY]		= "Antiquity",
	[RPG_PERIOD_EARLYMODERN]	= "Early-modern",
	[RPG_PERIOD_FUTURE]		= "Future",
	[RPG_PERIOD_MIDDLEAGES]		= "Middle ages",
	[RPG_PERIOD_MODERN]		= "Modern",
	[RPG_PERIOD_PREHISTORIC]	= "Pre-historic",
};

static const char *rpg_period_keys[RPG_PERIOD_COUNT] = {
	[RPG_PERIOD_ANTIQUITY]		= "ANTIQUITY",
	[RPG_PERIOD_EARLYMODERN]	= "EARLYMODERN",
	[RPG_PERIOD_FUTURE]		= "FUTURE",
	[RPG_PERIOD_MIDDLEAGES]		= "MIDDLEAGES",
	[RPG_PERIOD_MODERN]		= "MODERN",
	[RPG_PERIOD_PREHISTORIC]	= "PREHISTORIC",
};

static const char *rpg_genre_names[RPG_GENRE_COUNT] = {
	[RPG_GENRE_APOCALYPSE]		= "Apocalypse",
	[RPG_GENRE_ANIME]		= "Anime",
	[RPG_GENRE_ANTROPOMORPH]	= "Antropomorph",
	[RPG_GENRE_CYBERPUNK]		= "Cyberpunk",
	[RPG_GENRE_FANTASY]		= "Fantasy",
	[RPG_GENRE_HORROR]		= "Horror",
	[RPG_GENRE_MULTIGENRE]		= "Multi-genre",
	[RPG_GENRE_REALISTIC]		= "Realistic",
	[RPG_GENRE_SCIFI]		= "Sci-fi",
	[RPG_GENRE_STEAMPUNK]		= "Steampunk",
};

static const char *rpg_genre_keys[RPG_GENRE_COUNT] = {
	[RPG_GENRE_APOCALYPSE]		= "APOCALYPSE",
	[RPG_GENRE_ANIME]		= "ANIME",
	[RPG_GENRE_ANTROPOMORPH]	= "ANTROPOMORPH",
	[RPG_GENRE_CYBERPUNK]		= "CYBERPUNK",
	[RPG_GENRE_FANTASY]		= "FANTASY",
	[RPG_GENRE_HORROR]		= "HORROR",
	[RPG_GENRE_MULTIGENRE]		= "MULTIGENRE",
	[RPG_GENRE_REALISTIC]		= "REALISTIC",
	[RPG_GENRE_SCIFI]		= "SCIFI",
	[RPG_GENRE_STEAMPUNK]		= "STEAMPUNK",
};

static unsigned int lookup_flag(const char **flags, unsigned int count, const char *name)
{
	unsigned int i;

	for (i = 0; i < count; i++)
	{
		if (flags[i] != NULL && !strcasecmp(flags[i], name))
			return i;
	}
}

/***************************************************************************************/

service_t *rpgserv = NULL;
mowgli_list_t conf_rg_table;

void _modinit(module_t *m)
{
	rpgserv = service_add("rpgserv", NULL, &conf_rg_table);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_delete(rpgserv);
}
