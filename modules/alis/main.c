/*
 * Copyright (c) 2005-2007 William Pitcock, et al.
 * The rights to this code are as documented under doc/LICENSE.
 *
 * Copyright (C) 2003-2007 Lee Hardy <leeh@leeh.co.uk>
 * Copyright (C) 2003-2007 ircd-ratbox development team
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1.Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * 2.Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3.The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * ALIS, based on the ratbox-services implementation.
 *
 */

#include "atheme.h"
#include <limits.h>

DECLARE_MODULE_V1
(
	"alis/main", false, _modinit, _moddeinit,
	"$Revision$",
	"William Pitcock <nenolod -at- nenolod.net>"
);

#define ALIS_MAX_PARC	10
#define ALIS_MAX_MATCH	60

#define DIR_NONE	-1
#define DIR_UNSET	0
#define DIR_SET		1
#define DIR_EQUAL	2

service_t *alis;
mowgli_list_t alis_conftable;

static void alis_cmd_list(sourceinfo_t *si, int parc, char *parv[]);
static void alis_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t alis_list = { "LIST", "Lists channels matching given parameters.",
				AC_NONE, ALIS_MAX_PARC, alis_cmd_list, { .path = "alis/list" } };
command_t alis_help = { "HELP", "Displays contextual help information.",
				AC_NONE, 1, alis_cmd_help, { .path = "help" } };

struct alis_query
{
	char *mask;
	char *topic;
	int min;
	int max;
	int show_mode;
	int show_topicwho;
	unsigned int mode;
	int mode_dir;
	int mode_key;
	int mode_limit;
	int mode_ext[256];
	int skip;
	int maxmatches;
};

void _modinit(module_t *m)
{
	alis = service_add("alis", NULL, &alis_conftable);
	service_bind_command(alis, &alis_list);
	service_bind_command(alis, &alis_help);
}

void _moddeinit()
{
	service_unbind_command(alis, &alis_list);
	service_unbind_command(alis, &alis_help);

	service_delete(alis);
}

static int alis_parse_mode(const char *text, int *key, int *limit, int *ext)
{
	int mode = 0, i;

	if(!text)
		return 0;

	while(*text)
	{
		switch(*text)
		{
			case 'l':
				*limit = 1;
				break;
			case 'k':
				*key = 1;
				break;
			default:
				mode |= mode_to_flag(*text);
				for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
					if (*text == ignore_mode_list[i].mode)
						ext[i] = 1;
				break;
		}

		text++;
	}

	return mode;
}

static int parse_alis(sourceinfo_t *si, int parc, char *parv[], struct alis_query *query)
{
	int i = 0;
	char *opt = NULL, *arg = NULL;

	query->mode_dir = DIR_NONE;
	while ((opt = parv[i++]))
	{
		if(!strcasecmp(opt, "-min"))
		{
			if((arg = parv[i++]) == NULL || (query->min = atoi(arg)) < 1)
			{
				command_fail(si, fault_badparams, "Invalid -min option");
				return 0;
			}
		}
		else if(!strcasecmp(opt, "-max"))
		{
			if((arg = parv[i++]) == NULL || (query->max = atoi(arg)) < 1)
			{
				command_fail(si, fault_badparams, "Invalid -max option");
				return 0;
			}
		}
		else if(!strcasecmp(opt, "-maxmatches"))
		{
			if((arg = parv[i++]) == NULL || (query->maxmatches = atoi(arg)) == 0)
			{
				command_fail(si, fault_badparams, "Invalid -maxmatches option");
				return 0;
			}
			if(si->su != NULL && !has_priv(si, PRIV_CHAN_AUSPEX))
			{
				if(query->maxmatches > ALIS_MAX_MATCH)
				{
					command_fail(si, fault_badparams, "Invalid -maxmatches option");
					return 0;
				}
				if(query->maxmatches <= 0)
					query->maxmatches = ALIS_MAX_MATCH;
			}
			else
			{
				if(query->maxmatches <= 0)
					query->maxmatches = INT_MAX;
			}
		}
		else if(!strcasecmp(opt, "-skip"))
		{
			if((arg = parv[i++]) == NULL || (query->skip = atoi(arg)) < 1)
			{
				command_fail(si, fault_badparams, "Invalid -skip option");
				return 0;
			}
		}
		else if(!strcasecmp(opt, "-topic"))
		{
			query->topic = parv[i++];
			if (query->topic == NULL)
			{
				command_fail(si, fault_badparams, "Invalid -topic option");
				return 0;
			}
		}
		else if(!strcasecmp(opt, "-show"))
		{
			arg = parv[i++];

			if (arg == NULL)
			{
				command_fail(si, fault_badparams, "Invalid -show option");
				return 0;
			}

			if(arg[0] == 'm')
			{
				query->show_mode = 1;

				if(arg[1] == 't')
					query->show_topicwho = 1;
			}
			else if(arg[0] == 't')
			{
				query->show_topicwho = 1;

				if(arg[1] == 'm')
					query->show_mode = 1;
			}
		}
		else if(!strcasecmp(opt, "-mode"))
		{
			arg = parv[i++];

			if (arg == NULL)
			{
				command_fail(si, fault_badparams, "Invalid -mode option");
				return 0;
			}

			switch(*arg)
			{
				case '+':
					query->mode_dir = DIR_SET;
					break;
				case '-':
					query->mode_dir = DIR_UNSET;
					break;
				case '=':
					query->mode_dir = DIR_EQUAL;
					break;
				default:
					command_fail(si, fault_badparams, "Invalid -mode option");
					return 0;
			}

			query->mode = alis_parse_mode(arg+1, 
					&query->mode_key, 
					&query->mode_limit,
					query->mode_ext);
		}
		else
		{
			command_fail(si, fault_badparams, "Invalid option %s", opt);
			return 0;
		}
	}

	return 1;
}

static void print_channel(sourceinfo_t *si, channel_t *chptr, struct alis_query *query)
{
	int show_topicwho = query->show_topicwho;
	int show_topic = 1;

        /* cant show a topicwho, when a channel has no topic. */
        if(!chptr->topic)
	{
                show_topicwho = 0;
		show_topic = 0;
	}

	if(query->show_mode && show_topicwho && show_topic)
		command_success_nodata(si, "%-50s %-8s %3zu :%s (%s)",
			chptr->name, channel_modes(chptr, false),
			MOWGLI_LIST_LENGTH(&chptr->members),
			chptr->topic, chptr->topic_setter);
	else if(query->show_mode && show_topic)
		command_success_nodata(si, "%-50s %-8s %3zu :%s",
			chptr->name, channel_modes(chptr, false),
			MOWGLI_LIST_LENGTH(&chptr->members),
			chptr->topic);
	else if(query->show_mode)
		command_success_nodata(si, "%-50s %-8s %3zu",
			chptr->name, channel_modes(chptr, false),
			MOWGLI_LIST_LENGTH(&chptr->members));
	else if(show_topicwho && show_topic)
		command_success_nodata(si, "%-50s %3zu :%s (%s)",
			chptr->name, MOWGLI_LIST_LENGTH(&chptr->members),
			chptr->topic, chptr->topic_setter);
	else if(show_topic)
		command_success_nodata(si, "%-50s %3zu :%s",
			chptr->name, MOWGLI_LIST_LENGTH(&chptr->members),
			chptr->topic);
	else
		command_success_nodata(si, "%-50s %3zu",
			chptr->name, MOWGLI_LIST_LENGTH(&chptr->members));
}

static int show_channel(channel_t *chptr, struct alis_query *query)
{
	int i;

        /* skip +s channels */
        if(chptr->modes & CMODE_SEC)
                return 0;

        if((int)MOWGLI_LIST_LENGTH(&chptr->members) < query->min ||
           (query->max && (int)MOWGLI_LIST_LENGTH(&chptr->members) > query->max))
                return 0;

	if(query->mode_dir == DIR_SET)
	{
		if(((chptr->modes & query->mode) != query->mode) ||
		   (query->mode_key && chptr->key == NULL) ||
		   (query->mode_limit && !chptr->limit))
			return 0;
		for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
			if (query->mode_ext[i] && !chptr->extmodes[i])
				return 0;
	}
	else if(query->mode_dir == DIR_UNSET)
	{
		if((chptr->modes & query->mode) ||
		   (query->mode_key && chptr->key != NULL) ||
		   (query->mode_limit && chptr->limit))
			return 0;
		for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
			if (query->mode_ext[i] && chptr->extmodes[i])
				return 0;
	}
	else if(query->mode_dir == DIR_EQUAL)
	{
		if(((chptr->modes & ~(CMODE_LIMIT | CMODE_KEY)) != query->mode) ||
		   (query->mode_key && chptr->key == NULL) ||
		   (query->mode_limit && !chptr->limit))
			return 0;
		for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
			if (query->mode_ext[i] && !chptr->extmodes[i])
				return 0;
	}

        if(match(query->mask, chptr->name))
                return 0;

        if(query->topic != NULL && match(query->topic, chptr->topic))
                return 0;

        if(query->skip)
        {
                query->skip--;
                return 0;
        }

        return 1;
}

static void alis_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *chptr;
	struct alis_query query;
	mowgli_patricia_iteration_state_t state;
	int maxmatch;

	memset(&query, 0, sizeof(struct alis_query));
	query.maxmatches = ALIS_MAX_MATCH;

        query.mask = parc >= 1 ? parv[0] : "*";
	if (!parse_alis(si, parc - 1, parv + 1, &query))
		return;

	logcommand(si, CMDLOG_GET, "LIST: \2%s\2", query.mask);

	maxmatch = query.maxmatches;
	command_success_nodata(si,
		"Returning maximum of %d channel names matching '\2%s\2'",
		query.maxmatches, query.mask);

        /* hunting for one channel.. */
        if(strchr(query.mask, '*') == NULL && strchr(query.mask, '?') == NULL)
        {
                if((chptr = channel_find(query.mask)) != NULL)
                {
                        if(!(chptr->modes & CMODE_SEC) ||
					(si->su != NULL &&
					 chanuser_find(chptr, si->su)))
                                print_channel(si, chptr, &query);
                }

		command_success_nodata(si, "End of output");
                return;
        }

	MOWGLI_PATRICIA_FOREACH(chptr, &state, chanlist)
	{
		/* matches, so show it */
		if(show_channel(chptr, &query))
		{
			print_channel(si, chptr, &query);

			if(--maxmatch == 0)
			{
				command_success_nodata(si, "Maximum channel output reached");
				break;
			}
		}
	}

	command_success_nodata(si, "End of output");
        return;
}

static void alis_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	const char *command = parv[0];

	if (command == NULL)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), alis->nick);
		command_success_nodata(si, _("\2%s\2 allows searching for channels with more\n"
					"flexibility than the /list command."),
				alis->nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information on a command, type:"));
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == false) ? "msg " : "", alis->disp);
		command_success_nodata(si, " ");
		command_help(si, alis->commands);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
		return;
	}

	help_display(si, si->service, command, alis->commands);
}
