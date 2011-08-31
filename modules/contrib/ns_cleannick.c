/*
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Clean obnoxious nicknames, such as LaMENiCK -> lamenick.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/ns_cleannick", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

#define	LAMENESS_WEIGHT		0.35f

/*
 * Determine if a nickname is lame.  Non-alphabetical characters
 * are penalized twice, uppercase characters are penalized once.
 */
static bool is_nickname_lame(const char *nickname)
{
	const char *p;
	unsigned int capcount = 0;
	float score;

	return_val_if_fail(nickname != NULL, false);

	for (p = nickname; *p != '\0'; p++)
	{
		if (IsUpper(*p))
			capcount++;

#ifdef NOTYET
		if (!IsAlpha(*p))
			capcount += 2;
#endif
	}

	score = (float) capcount / (float) strlen(nickname);
	slog(LG_DEBUG, "is_nickname_lame(%s): score %0.3f %d/%d caps", nickname, score, capcount, strlen(nickname));

	if (score > LAMENESS_WEIGHT)
		return true;

	return false;
}

/*
 * Sanitize a nickname and then change it to the sanitized version forcefully.
 */
static void clean_nickname(user_t *u)
{
	char nickbuf[NICKLEN];
	char *p;

	return_if_fail(u != NULL);

	mowgli_strlcpy(nickbuf, u->nick, NICKLEN);

	p = nickbuf;

	while (*p++)
	{
		if (IsUpper(*p))
			*p = ToLower(*p);		
	}

	if (is_nickname_lame(nickbuf))
	{
		slog(LG_DEBUG, "clean_nickname(%s): cleaned nickname %s is still lame", u->nick, nickbuf);
		return;
	}

	notice(nicksvs.nick, u->nick, "Your nick has been changed to \2%s\2 per %s nickname quality guidelines.",
	       nickbuf, me.netname);

	fnc_sts(nicksvs.me->me, u, nickbuf, FNC_FORCE);	
}

static void user_state_changed(hook_user_nick_t *data)
{
	return_if_fail(data != NULL);
	return_if_fail(data->u != NULL);

	if (is_nickname_lame(data->u->nick))
	{
#ifdef TAUNT_LAME_USERS
		if (data->oldnick != NULL && !is_nickname_lame(data->oldnick))
			notice(nicksvs.nick, data->u->nick, "\2%s\2 was much less lame, \2%s\2.",
			       data->oldnick, data->u->nick);
#endif

		clean_nickname(data->u);
	}
}

void _modinit(module_t *m)
{
	hook_add_event("user_add");
	hook_add_user_add(user_state_changed);

	hook_add_event("user_nickchange");
	hook_add_user_nickchange(user_state_changed);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_user_add(user_state_changed);
	hook_del_user_nickchange(user_state_changed);
}
