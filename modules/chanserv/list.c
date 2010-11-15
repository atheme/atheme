/*
 * Copyright (c) 2005 Robin Burchell, et al.
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * List chanserv-controlled channels.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/list", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_list(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_list = { "LIST", N_("Lists channels registered matching a given pattern."), PRIV_CHAN_AUSPEX, 10, cs_cmd_list, { .path = "cservice/list" } };

void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_list);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_list);
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

static void cs_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	metadata_t *md, *mdclosed;
	char *chanpattern = NULL, *markpattern = NULL, *closedpattern = NULL;
	char buf[BUFSIZE];
	char criteriastr[BUFSIZE];
	unsigned int matches = 0;
	unsigned int flagset = 0;
	int aclsize = 0;
	time_t age = 0, lastused = 0;
	bool closed = false, marked = false, markmatch, closedmatch;
	mowgli_patricia_iteration_state_t state;
	list_option_t optstable[] = {
		{"pattern",	OPT_STRING,	{.strval = &chanpattern}, 0},
		{"mark-reason", OPT_STRING,	{.strval = &markpattern}, 0},
		{"close-reason", OPT_STRING,    {.strval = &closedpattern}, 0},
		{"noexpire",	OPT_FLAG,	{.flagval = &flagset}, MC_HOLD},
		{"held",	OPT_FLAG,	{.flagval = &flagset}, MC_HOLD},
		{"hold",	OPT_FLAG,	{.flagval = &flagset}, MC_HOLD},
		{"noop",	OPT_FLAG,	{.flagval = &flagset}, MC_NOOP},
		{"limitflags",	OPT_FLAG,	{.flagval = &flagset}, MC_LIMITFLAGS},
		{"secure",	OPT_FLAG,	{.flagval = &flagset}, MC_SECURE},
		{"verbose",	OPT_FLAG,	{.flagval = &flagset}, MC_VERBOSE},
		{"restricted",	OPT_FLAG,	{.flagval = &flagset}, MC_RESTRICTED},
		{"keeptopic",	OPT_FLAG,	{.flagval = &flagset}, MC_KEEPTOPIC},
		{"verbose-ops",	OPT_FLAG,	{.flagval = &flagset}, MC_VERBOSE_OPS},
		{"topiclock",	OPT_FLAG,	{.flagval = &flagset}, MC_TOPICLOCK},
		{"guard",	OPT_FLAG,	{.flagval = &flagset}, MC_GUARD},
		{"private",	OPT_FLAG,	{.flagval = &flagset}, MC_PRIVATE},
		{"closed",	OPT_BOOL,	{.boolval = &closed}, 0},
		{"marked",	OPT_BOOL,	{.boolval = &marked}, 0},
		{"aclsize",	OPT_INT,	{.intval = &aclsize}, 0},
		{"registered",	OPT_AGE,	{.ageval = &age}, 0},
		{"lastused",	OPT_AGE,	{.ageval = &lastused}, 0},
	};

	process_parvarray(optstable, ARRAY_SIZE(optstable), parc, parv);
	build_criteriastr(criteriastr, parc, parv);

	command_success_nodata(si, _("Channels matching \2%s\2:"), criteriastr);

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		if (chanpattern != NULL && match(chanpattern, mc->name))
			continue;

		if (markpattern)
		{
			markmatch = false;
			md = metadata_find(mc, "private:mark:reason");
			if (md != NULL && !match(markpattern, md->value))
				markmatch = true;

			if (!markmatch)
				continue;
		}

		if (closedpattern)
		{
			closedmatch = false;
			mdclosed = metadata_find(mc, "private:close:reason");
			if (mdclosed != NULL && !match(closedpattern, mdclosed->value))
				closedmatch = true;

			if (!closedmatch)
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

		/* in the future we could add a LIMIT parameter */
		*buf = '\0';

		if (metadata_find(mc, "private:mark:setter")) {
			strlcat(buf, "\2[marked]\2", BUFSIZE);
		}
		if (metadata_find(mc, "private:close:closer")) {
			if (*buf)
				strlcat(buf, " ", BUFSIZE);

			strlcat(buf, "\2[closed]\2", BUFSIZE);
		}
		if (mc->flags & MC_HOLD) {
			if (*buf)
				strlcat(buf, " ", BUFSIZE);

			strlcat(buf, "\2[held]\2", BUFSIZE);
		}

		command_success_nodata(si, "- %s (%s) %s", mc->name, mychan_founder_names(mc), buf);
		matches++;
	}

	logcommand(si, CMDLOG_ADMIN, "LIST: \2%s\2 (\2%d\2 matches)", criteriastr, matches);
	if (matches == 0)
		command_success_nodata(si, _("No channel matched criteria \2%s\2"), criteriastr);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 match for criteria \2%s\2"), N_("\2%d\2 matches for criteria \2%s\2"), matches), matches, criteriastr);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
