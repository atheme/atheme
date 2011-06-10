/* prettyprint helpers for rpgserv */

#ifndef __RPGSERV__PRETTYPRINT_H__GUARD
#define __RPGSERV__PRETTYPRINT_H__GUARD

/*
 * Important: keys and names must line up index-wise or everything
 * is going to go boom.  ARRAY_SIZE() must also match, but we assert
 * on that.
 */

static const char *genre_keys[] = {
	"apocalypse", "anime", "antropomorph", "cyberpunk",
	"fantasy", "horror", "multigenre", "realistic",
	"scifi", "steampunk", "other"
};

static const char *genre_names[] = {
	"Apocalypse", "Anime", "Anthromorph", "Cyberpunk",
	"Fantasy", "Horror", "Multi-genre", "Realistic",
	"Sci-Fi", "Steampunk", "Other"
};

static const char *period_keys[] = {
	"prehistoric", "antiquity", "middleages", "earlymodern",
	"modern", "future"
};

static const char *period_names[] = {
	"Prehistoric", "Antiquity", "Middle Ages", "Early Modern",
	"Modern", "Future"
};

static const char *ruleset_keys[] = {
	"adnd", "homebrew", "dnd3.0", "dnd3.5", "dnd4.0", "freeform",
	"other", "owod", "nwod"
};

static const char *ruleset_names[] = {
	"Advanced Dungeons & Dragons", "Homebrew", "Dungeons & Dragons 3.0",
	"Dungeons & Dragons 3.5", "Dungeons & Dragons 4.0", "Freeform",
	"Other", "Old World of Darkness", "New World of Darkness"
};

static const char *rating_keys[] = {
	"g", "pg", "pg13", "r", "adult"
};

static const char *rating_names[] = {
	"G", "PG", "PG-13", "R", "Adult"
};

static const char *system_keys[] = {
	"charapproval", "diced", "sheeted"
};

static const char *system_names[] = {
	"Character Approval", "Diced", "Character Sheets"
};

#endif
