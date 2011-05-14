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

static const char *rpg_period_names[RPG_PERIOD_COUNT] = {
	[RPG_PERIOD_ANTIQUITY]		= "Antiquity",
	[RPG_PERIOD_EARLYMODERN]	= "Early-modern",
	[RPG_PERIOD_FUTURE]		= "Future",
	[RPG_PERIOD_MIDDLEAGES]		= "Middle ages",
	[RPG_PERIOD_MODERN]		= "Modern",
	[RPG_PERIOD_PREHISTORIC]	= "Pre-historic",
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

static unsigned int lookup_flag(const char **flags, unsigned int count, const char *name)
{
	unsigned int i;

	for (i = 0; i < count; i++)
	{
		if (flags[i] != NULL && !strcasecmp(flags[i], name))
			return i;
	}
}


