/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Robin Burchell, et al.
 * Copyright (C) 2010 William Pitcock <nenolod@dereferenced.org>
 *
 * List chanserv-controlled channels.
 */

#include <atheme.h>

enum list_opttype
{
	OPT_BOOL,
	OPT_INT,
	OPT_STRING,
	OPT_FLAG,
	OPT_AGE,
};

struct list_option
{
	const char *option;
	enum list_opttype opttype;
	union {
		bool *boolval;
		int *intval;
		const char **strval;
		unsigned int *flagval;
		time_t *ageval;
	} optval;
	unsigned int flag;
};

static time_t
parse_age(const char *s)
{
	time_t duration;

	duration = (atol(s) * 60);
	while (isdigit((unsigned char)*s))
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

static void
process_parvarray(const struct list_option *opts, size_t optsize, int parc, char *parv[])
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
				}
			}
		}
	}
}

static void
build_criteriastr(char *buf, int parc, char *parv[])
{
	int i;

	return_if_fail(buf != NULL);

	*buf = 0;
	for (i = 0; i < parc; i++)
	{
		mowgli_strlcat(buf, parv[i], BUFSIZE);
		mowgli_strlcat(buf, " ", BUFSIZE);
	}
}

static bool
check_extmlock(const struct metadata *md, const bool *modes, bool on)
{
	for (size_t i = 0; i < ignore_mode_list_size; i++)
	{
		if (!modes[i])
			continue;

		if (!md)
			return false;

		const char *p = md->value;
		while (*p != '\0')
		{
			if (*p == ignore_mode_list[i].mode)
			{
				if (on && (p[1] == ' ' || p[1] == '\0'))
					return false;
				else if (!on && (p[1] != ' ' && p[1] != '\0'))
					return false;
			}

			while (*p != ' ' && *p != '\0')
				p++;
			while (*p == ' ')
				p++;
		}
	}
	return true;
}

static void
cs_cmd_list(struct sourceinfo *si, int parc, char *parv[])
{
	const char *chanpattern = NULL, *markpattern = NULL, *closedpattern = NULL, *mlock = NULL;
	unsigned int flagset = 0;
	int aclsize = 0;
	time_t age = 0, lastused = 0;
	bool closed = false, marked = false;
	struct list_option optstable[] = {
		{"pattern",      OPT_STRING,    {.strval = &chanpattern}, 0},
		{"mark-reason",  OPT_STRING,    {.strval = &markpattern}, 0},
		{"close-reason", OPT_STRING,    {.strval = &closedpattern}, 0},
		{"noexpire",     OPT_FLAG,      {.flagval = &flagset}, MC_HOLD},
		{"held",         OPT_FLAG,      {.flagval = &flagset}, MC_HOLD},
		{"hold",         OPT_FLAG,      {.flagval = &flagset}, MC_HOLD},
		{"noop",         OPT_FLAG,      {.flagval = &flagset}, MC_NOOP},
		{"limitflags",   OPT_FLAG,      {.flagval = &flagset}, MC_LIMITFLAGS},
		{"secure",       OPT_FLAG,      {.flagval = &flagset}, MC_SECURE},
		{"nosync",       OPT_FLAG,      {.flagval = &flagset}, MC_NOSYNC},
		{"verbose",      OPT_FLAG,      {.flagval = &flagset}, MC_VERBOSE},
		{"restricted",   OPT_FLAG,      {.flagval = &flagset}, MC_RESTRICTED},
		{"keeptopic",    OPT_FLAG,      {.flagval = &flagset}, MC_KEEPTOPIC},
		{"verbose-ops",  OPT_FLAG,      {.flagval = &flagset}, MC_VERBOSE_OPS},
		{"topiclock",    OPT_FLAG,      {.flagval = &flagset}, MC_TOPICLOCK},
		{"guard",        OPT_FLAG,      {.flagval = &flagset}, MC_GUARD},
		{"private",      OPT_FLAG,      {.flagval = &flagset}, MC_PRIVATE},
		{"pubacl",       OPT_FLAG,      {.flagval = &flagset}, MC_PUBACL},
		{"mlock",        OPT_STRING,    {.strval = &mlock}, 0},
		{"closed",       OPT_BOOL,      {.boolval = &closed}, 0},
		{"marked",       OPT_BOOL,      {.boolval = &marked}, 0},
		{"aclsize",      OPT_INT,       {.intval = &aclsize}, 0},
		{"registered",   OPT_AGE,       {.ageval = &age}, 0},
		{"lastused",     OPT_AGE,       {.ageval = &lastused}, 0},
	};

	// This isn't a channel-specific command. Exclude it from fantasy;
	// this also allows bots to react to it without us interfering,
	// cf chanserv/register for precedent
	if (si->c != NULL)
		return;

	process_parvarray(optstable, ARRAY_SIZE(optstable), parc, parv);

	char criteriastr[BUFSIZE];
	build_criteriastr(criteriastr, parc, parv);

	unsigned int mlock_on = 0, mlock_off = 0;
	bool mlock_key = false, mlock_limit = false;
	bool extmlock_on[ignore_mode_list_size];
	bool extmlock_off[ignore_mode_list_size];
	memset(extmlock_on, 0, sizeof extmlock_on);
	memset(extmlock_off, 0, sizeof extmlock_off);

	if (mlock)
	{
		int dir = MTYPE_NUL;

		for (const char *c = mlock; *c; c++)
		{
			int flag;
			switch (*c)
			{
				case '+':
					dir = MTYPE_ADD;
					break;

				case '-':
					dir = MTYPE_DEL;
					break;

				case 'l':
					if (dir == MTYPE_DEL)
						mlock_off |= CMODE_LIMIT;
					else
						mlock_limit = true;
					break;

				case 'k':
					if (dir == MTYPE_DEL)
						mlock_off |= CMODE_KEY;
					else
						mlock_key = true;
					break;

				default:
					flag = mode_to_flag(*c);
					if (flag)
					{
						if (dir == MTYPE_DEL)
							mlock_off |= flag;
						else
							mlock_on |= flag;
					}
					else
					{
						size_t i;
						for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
						{
							if (*c == ignore_mode_list[i].mode)
								break;
						}

						if (ignore_mode_list[i].mode == '\0')
							continue;

						if (dir == MTYPE_DEL)
							extmlock_off[i] = true;
						else
							extmlock_on[i] = true;
					}
					break;
			}
		}
	}

	command_success_nodata(si, _("Channels matching \2%s\2:"), criteriastr);

	unsigned int matches = 0;

	struct mychan *mc;
	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		if (chanpattern != NULL && match(chanpattern, mc->name))
			continue;

		if (markpattern)
		{
			const struct metadata *md = metadata_find(mc, "private:mark:reason");
			if (md == NULL || match(markpattern, md->value) != 0)
				continue;
		}

		if (closedpattern)
		{
			const struct metadata *md = metadata_find(mc, "private:close:reason");
			if (md == NULL || match(closedpattern, md->value) != 0)
				continue;
		}

		if (marked && !metadata_find(mc, "private:mark:setter"))
			continue;

		if (closed && !metadata_find(mc, "private:close:closer"))
			continue;

		if (flagset && (mc->flags & flagset) != flagset)
			continue;

		if (aclsize && MOWGLI_LIST_LENGTH(&mc->chanacs) < (unsigned int)aclsize)
			continue;

		if (age && (CURRTIME - mc->registered) < age)
			continue;

		if (lastused && (CURRTIME - mc->used) < lastused)
			continue;

		if ((mlock_on & mc->mlock_on) != mlock_on)
			continue;

		if ((mlock_off & mc->mlock_off) != mlock_off)
			continue;

		if (mlock_key && !mc->mlock_key)
			continue;

		if (mlock_limit && !mc->mlock_limit)
			continue;

		const struct metadata *extmlock_md = metadata_find(mc, "private:mlockext");

		if (!check_extmlock(extmlock_md, extmlock_on, true))
			continue;

		if (!check_extmlock(extmlock_md, extmlock_off, false))
			continue;

		// in the future we could add a LIMIT parameter
		char buf[BUFSIZE] = { 0 };

		if (metadata_find(mc, "private:mark:setter")) {
			mowgli_strlcat(buf, "\2[marked]\2", BUFSIZE);
		}
		if (metadata_find(mc, "private:close:closer")) {
			if (*buf)
				mowgli_strlcat(buf, " ", BUFSIZE);

			mowgli_strlcat(buf, "\2[closed]\2", BUFSIZE);
		}
		if (mc->flags & MC_HOLD) {
			if (*buf)
				mowgli_strlcat(buf, " ", BUFSIZE);

			mowgli_strlcat(buf, "\2[held]\2", BUFSIZE);
		}

		command_success_nodata(si, "- %s (%s) %s", mc->name, mychan_founder_names(mc), buf);
		matches++;
	}

	logcommand(si, CMDLOG_ADMIN, "LIST: \2%s\2 (\2%u\2 matches)", criteriastr, matches);
	if (matches == 0)
		command_success_nodata(si, _("No channel matched criteria \2%s\2"), criteriastr);
	else
		command_success_nodata(si, ngettext(N_("\2%u\2 match for criteria \2%s\2."),
		                                    N_("\2%u\2 matches for criteria \2%s\2."),
		                                    matches), matches, criteriastr);
}

static struct command cs_list = {
	.name           = "LIST",
	.desc           = N_("Lists channels registered matching a given pattern."),
	.access         = PRIV_CHAN_AUSPEX,
	.maxparc        = 10,
	.cmd            = &cs_cmd_list,
	.help           = { .path = "cservice/list" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	service_named_bind_command("chanserv", &cs_list);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_list);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/list", MODULE_UNLOAD_CAPABILITY_OK)
