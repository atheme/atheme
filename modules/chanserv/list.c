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

command_t cs_list = { "LIST", N_("Lists channels registered matching a given pattern."), PRIV_CHAN_AUSPEX, 10, cs_cmd_list };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_list, cs_cmdtree);
	help_addentry(cs_helptree, "LIST", "help/cservice/list", NULL);
}

void _moddeinit()
{
	command_delete(&cs_list, cs_cmdtree);
	help_delentry(cs_helptree, "LIST");
}

typedef enum {
	OPT_BOOL,
	OPT_INT,
	OPT_STRING,
	OPT_FLAG,
} list_opttype_t;

typedef struct {
	char *option;
	list_opttype_t opttype;
	union {
		bool *boolval;
		int *intval;
		char **strval;
		unsigned int *flagval;
	} optval;
	unsigned int flag;
} list_option_t;

static void process_parvarray(list_option_t *opts, size_t optsize, int parc, char *parv[])
{
	int i, j;

	for (i = 0; i < parc; i++)
	{
		for (j = 0; j < optsize; j++)
		{
			slog(LG_INFO, "i: %s i+1: %s j: %s <TEST>", parv[i], parv[i + 1], opts[j].option);

			if (!strcasecmp(opts[j].option, parv[i]))
			{
				slog(LG_INFO, "i: %s i+1: %s j: %s <MATCH>", parv[i], parv[i + 1], opts[j].option);

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
				default:
					break;
				}
			}
		}
	}
}

static void cs_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char *chanpattern;
	char buf[BUFSIZE];
	unsigned int matches = 0;
	unsigned int flagset = 0;
	bool closed = false, marked = false;
	mowgli_patricia_iteration_state_t state;
	list_option_t optstable[] = {
		{"pattern",	OPT_STRING,	{.strval = &chanpattern}},
		{"noexpire",	OPT_FLAG,	{.flagval = &flagset}, MU_HOLD},
		{"hold",	OPT_FLAG,	{.flagval = &flagset}, MU_HOLD},
		{"closed",	OPT_BOOL,	{.boolval = &closed}},
		{"marked",	OPT_BOOL,	{.boolval = &marked}},
	};

	process_parvarray(optstable, ARRAY_SIZE(optstable), parc, parv);

	command_success_nodata(si, _("Channels matching specified criteria:"));

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		if (chanpattern != NULL && match(chanpattern, mc->name))
			continue;

		if (marked && !metadata_find(mc, "private:mark:setter"))
			continue;

		if (closed && !metadata_find(mc, "private:close:closer"))
			continue;

		if (flagset && !(mc->flags & flagset))
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

	logcommand(si, CMDLOG_ADMIN, "LIST: \2%s\2 (%d matches)", chanpattern, matches);
	if (matches == 0)
		command_success_nodata(si, _("No channel matched pattern \2%s\2"), chanpattern);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 match for pattern \2%s\2"), N_("\2%d\2 matches for pattern \2%s\2"), matches), matches, chanpattern);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
