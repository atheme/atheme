/*
 * Copyright (c) 2005-2007 Robin Burchell, et al.
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LIST function.
 * Based on Alex Lambert's LISTEMAIL.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/list", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_list(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_list = { "LIST", N_("Lists nicknames registered matching a given pattern."), PRIV_USER_AUSPEX, 10, ns_cmd_list, { .path = "nickserv/list" } };

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_list);
}

void _moddeinit()
{
	service_named_unbind_command("nickserv", &ns_list);
}

typedef enum {
	OPT_BOOL,
	OPT_INT,
	OPT_STRING,
	OPT_FLAG,
	OPT_AGE,
} list_opttype_t;

typedef struct {
	char *option;
	list_opttype_t opttype;
	union {
		bool *boolval;
		int *intval;
		char **strval;
		unsigned int *flagval;
		time_t *ageval;
	} optval;
	unsigned int flag;
} list_option_t;

static time_t parse_age(char *s)
{
	time_t duration;

	duration = (atol(s) * 60);
	while (isdigit(*s))
		s++;

	if (*s == 'h' || *s == 'H')
		duration *= 60;
	else if (*s == 'd' || *s == 'D')
		duration *= 1440;
	else if (*s == 'w' || *s == 'W')
		duration *= 10080;
	else if (*s == '\0')
		;
	else
		duration = 0;

	return duration;
}

static void process_parvarray(list_option_t *opts, size_t optsize, int parc, char *parv[])
{
	int i;
	size_t j;

	for (i = 0; i < parc; i++)
	{
		for (j = 0; j < optsize; j++)
		{
			if (!strcasecmp(opts[j].option, parv[i]))
			{
				switch(opts[j].opttype)
				{
				case OPT_BOOL:
					*opts[j].optval.boolval = true;
					break;
				case OPT_INT:
					if (i + 1 < parc)
					{
						*opts[j].optval.intval = atoi(parv[i + 1]);
						i++;
					}
					break;
				case OPT_STRING:
					if (i + 1 < parc)
					{
						*opts[j].optval.strval = parv[i + 1];
						i++;
					}
					break;
				case OPT_FLAG:
					*opts[j].optval.flagval |= opts[j].flag;
					break;
				case OPT_AGE:
					if (i + 1 < parc)
					{
						*opts[j].optval.ageval = parse_age(parv[i + 1]);
						i++;
					}
					break;					
				default:
					break;
				}
			}
		}
	}
}

static void build_criteriastr(char *buf, int parc, char *parv[])
{
	int i;

	return_if_fail(buf != NULL);

	*buf = 0;
	for (i = 0; i < parc; i++)
	{
		strlcat(buf, parv[i], BUFSIZE);
		strlcat(buf, " ", BUFSIZE);
	}
}

static void list_one(sourceinfo_t *si, myuser_t *mu, mynick_t *mn)
{
	char buf[BUFSIZE];

	if (mn != NULL)
		mu = mn->owner;

	*buf = '\0';
	if (metadata_find(mu, "private:freeze:freezer")) {
		if (*buf)
			strlcat(buf, " ", BUFSIZE);

		strlcat(buf, "\2[frozen]\2", BUFSIZE);
	}
	if (metadata_find(mu, "private:mark:setter")) {
		if (*buf)
			strlcat(buf, " ", BUFSIZE);

		strlcat(buf, "\2[marked]\2", BUFSIZE);
	}
	if (mu->flags & MU_HOLD) {
		if (*buf)
			strlcat(buf, " ", BUFSIZE);

		strlcat(buf, "\2[held]\2", BUFSIZE);
	}
	if (mu->flags & MU_WAITAUTH) {
		if (*buf)
			strlcat(buf, " ", BUFSIZE);

		strlcat(buf, "\2[unverified]\2", BUFSIZE);
	}

	if (mn == NULL || !irccasecmp(mn->nick, entity(mu)->name))
		command_success_nodata(si, "- %s (%s) %s", entity(mu)->name, mu->email, buf);
	else
		command_success_nodata(si, "- %s (%s) (%s) %s", mn->nick, mu->email, entity(mu)->name, buf);
}

static void ns_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	char criteriastr[BUFSIZE];
	char pat[512], *pattern = NULL, *nickpattern = NULL, *hostpattern = NULL, *p, *email = NULL, *markpattern = NULL, *frozenpattern = NULL;
	bool hostmatch, markmatch, frozenmatch;
	mowgli_patricia_iteration_state_t state;
	myentity_iteration_state_t mestate;
	myuser_t *mu;
	myentity_t *mt;
	mynick_t *mn;
	metadata_t *md, *mdmark, *mdfrozen;
	int matches = 0;
	bool frozen = false, marked = false;
	unsigned int flagset = 0;
	time_t age = 0, lastlogin = 0;

	list_option_t optstable[] = {
		{"pattern",	OPT_STRING,	{.strval = &pattern}, 0},
		{"email",	OPT_STRING,	{.strval = &email}, 0},
		{"mail",	OPT_STRING,	{.strval = &email}, 0},
		{"mark-reason", OPT_STRING,	{.strval = &markpattern}, 0},
		{"frozen-reason", OPT_STRING,   {.strval = &frozenpattern}, 0},
		{"noexpire",	OPT_FLAG,	{.flagval = &flagset}, MU_HOLD},
		{"held",	OPT_FLAG,	{.flagval = &flagset}, MU_HOLD},
		{"hold",	OPT_FLAG,	{.flagval = &flagset}, MU_HOLD},
		{"noop",	OPT_FLAG,	{.flagval = &flagset}, MU_NOOP},
		{"neverop",	OPT_FLAG,	{.flagval = &flagset}, MU_NEVEROP},
		{"waitauth",	OPT_FLAG,	{.flagval = &flagset}, MU_WAITAUTH},
		{"hidemail",	OPT_FLAG,	{.flagval = &flagset}, MU_HIDEMAIL},
		{"nomemo",	OPT_FLAG,	{.flagval = &flagset}, MU_NOMEMO},
		{"emailmemos",	OPT_FLAG,	{.flagval = &flagset}, MU_EMAILMEMOS},
		{"use-privmsg",	OPT_FLAG,	{.flagval = &flagset}, MU_USE_PRIVMSG},
		{"private",	OPT_FLAG,	{.flagval = &flagset}, MU_PRIVATE},
		{"quietchg",	OPT_FLAG,	{.flagval = &flagset}, MU_QUIETCHG},
		{"nogreet",	OPT_FLAG,	{.flagval = &flagset}, MU_NOGREET},
		{"regnolimit",	OPT_FLAG,	{.flagval = &flagset}, MU_REGNOLIMIT},
		{"frozen",	OPT_BOOL,	{.boolval = &frozen}, 0},
		{"marked",	OPT_BOOL,	{.boolval = &marked}, 0},
		{"registered",	OPT_AGE,	{.ageval = &age}, 0},
		{"lastlogin",	OPT_AGE,	{.ageval = &lastlogin}, 0},
	};

	process_parvarray(optstable, ARRAY_SIZE(optstable), parc, parv);
	build_criteriastr(criteriastr, parc, parv);

	if (pattern != NULL)
	{
		strlcpy(pat, pattern, sizeof pat);
		p = strrchr(pat, ' ');
		if (p == NULL)
			p = strrchr(pat, '!');
		if (p != NULL)
		{
			*p++ = '\0';
			nickpattern = pat;
			hostpattern = p;
		}
		else if (strchr(pat, '@'))
			hostpattern = pat;
		else
			nickpattern = pat;
		if (nickpattern && !strcmp(nickpattern, "*"))
			nickpattern = NULL;
	}

	if (nicksvs.no_nick_ownership)
	{
		MYENTITY_FOREACH_T(mt, &mestate, ENT_USER)
		{
			mu = user(mt);
			if (nickpattern && match(nickpattern, entity(mu)->name))
				continue;
			if (hostpattern)
			{
				hostmatch = false;
				md = metadata_find(mu, "private:host:actual");
				if (md != NULL && !match(hostpattern, md->value))
					hostmatch = true;
				md = metadata_find(mu, "private:host:vhost");
				if (md != NULL && !match(hostpattern, md->value))
					hostmatch = true;
				if (!hostmatch)
					continue;
			}

			if (email && match(email, mu->email))
				continue;

			if (markpattern)
			{
				markmatch = false;
				mdmark = metadata_find(mu, "private:mark:reason");
				if (mdmark != NULL && !match(markpattern, mdmark->value))
					markmatch = true;

				if (!markmatch)
					continue;
			}

			if (frozenpattern)
			{
				frozenmatch = false;
				mdfrozen = metadata_find(mu, "private:freeze:reason");
				if (mdfrozen != NULL && !match(frozenpattern, mdfrozen->value))
					frozenmatch = true;

				if (!frozenmatch)
					continue;
			}

			if (marked && !metadata_find(mu, "private:mark:setter"))
				continue;

			if (frozen && !metadata_find(mu, "private:freeze:freezer"))
				continue;

			if (flagset && (mu->flags & flagset) != flagset)
				continue;

			if (age && (CURRTIME - mu->registered) < age)
				continue;

			if (lastlogin && (CURRTIME - mu->lastlogin) < lastlogin)
				continue;

			list_one(si, mu, NULL);
			matches++;
		}
	}
	else
	{
		MOWGLI_PATRICIA_FOREACH(mn, &state, nicklist)
		{
			if (nickpattern && match(nickpattern, mn->nick))
				continue;
			mu = mn->owner;
			if (hostpattern)
			{
				hostmatch = false;
				md = metadata_find(mu, "private:host:actual");
				if (md != NULL && !match(hostpattern, md->value))
					hostmatch = true;
				md = metadata_find(mu, "private:host:vhost");
				if (md != NULL && !match(hostpattern, md->value))
					hostmatch = true;
				if (!hostmatch)
					continue;
			}

			if (email && match(email, mu->email))
				continue;

			if (markpattern)
			{
				markmatch = false;
				mdmark = metadata_find(mu, "private:mark:reason");
				if (mdmark != NULL && !match(markpattern, mdmark->value))
					markmatch = true;

				if (!markmatch)
					continue;
			}

			if (frozenpattern)
			{
				frozenmatch = false;
				mdfrozen = metadata_find(mu, "private:freeze:reason");
				if (mdfrozen != NULL && !match(frozenpattern, mdfrozen->value))
					frozenmatch = true;

				if (!frozenmatch)
					continue;
			}

			if (marked && !metadata_find(mu, "private:mark:setter"))
				continue;

			if (frozen && !metadata_find(mu, "private:freeze:freezer"))
				continue;

			if (flagset && (mu->flags & flagset) != flagset)
				continue;

			if (age && (CURRTIME - mu->registered) < age)
				continue;

			if (lastlogin && (CURRTIME - mu->lastlogin) < lastlogin)
				continue;

			list_one(si, NULL, mn);
			matches++;
		}
	}

	logcommand(si, CMDLOG_ADMIN, "LIST: \2%s\2 (\2%d\2 matches)", criteriastr, matches);
	if (matches == 0)
		command_success_nodata(si, _("No nicknames matched criteria \2%s\2"), criteriastr);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 match for criteria \2%s\2"), N_("\2%d\2 matches for criteria \2%s\2"), matches), matches, criteriastr);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
