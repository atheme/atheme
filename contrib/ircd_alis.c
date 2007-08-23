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
 * $Id$
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"alis", FALSE, _modinit, _moddeinit,
	"$Revision$",
	"William Pitcock <nenolod -at- nenolod.net>"
);

#define ALIS_MAX_PARC	10
#define ALIS_MAX_MATCH	60

#define DIR_UNSET	0
#define DIR_SET		1
#define DIR_EQUAL	2

service_t *alis;
list_t alis_cmdtree;

static void alis_cmd_list(sourceinfo_t *si, int parc, char *parv[]);
static void alis_cmd_help(sourceinfo_t *si, int parc, char *parv[]);
static void alis_handler(sourceinfo_t *si, int parc, char *parv[]);

command_t alis_list = { "LIST", "Lists channels matching given parameters.",
				AC_NONE, ALIS_MAX_PARC, alis_cmd_list };
command_t alis_help = { "HELP", "Displays contextual help information.",
				AC_NONE, 1, alis_cmd_help };

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
	int skip;
};

void _modinit(module_t *m)
{
	alis = add_service("ALIS", "alis", me.name, "Channel Directory", alis_handler, &alis_cmdtree);

	command_add(&alis_list, &alis_cmdtree);
	command_add(&alis_help, &alis_cmdtree);
}

void _moddeinit()
{
	command_delete(&alis_list, &alis_cmdtree);
	command_delete(&alis_help, &alis_cmdtree);

	del_service(alis);
}

static int alis_parse_mode(const char *text, int *key, int *limit)
{
	int mode = 0;

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
			query->topic = strtok(NULL, " ");
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
					&query->mode_limit);

			if(query->mode == 0)
			{
				command_fail(si, fault_badparams, "Invalid -mode option");
				return 0;
			}
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
	char modestr[MAXEXTMODES+60];
	int i, j;

        /* cant show a topicwho, when a channel has no topic. */
        if(!chptr->topic)
	{
                show_topicwho = 0;
		show_topic = 0;
	}
	if (query->show_mode)
	{
		j = 0;
		modestr[j++] = '+';
		for (i = 0; i < MAXEXTMODES; i++)
			if (chptr->extmodes[i] != NULL)
				modestr[j++] = ignore_mode_list[i].mode;
		if (chptr->limit != 0)
			modestr[j++] = 'l';
		if (chptr->key != NULL)
			modestr[j++] = 'k';
		modestr[j] = '\0';
		strlcat(modestr, flags_to_string(chptr->modes), sizeof modestr);
	}

	if(query->show_mode && show_topicwho && show_topic)
		command_success_nodata(si, "%-50s %-8s %3ld :%s (%s)",
			chptr->name, modestr,
			LIST_LENGTH(&chptr->members),
			chptr->topic, chptr->topic_setter);
	else if(query->show_mode)
		command_success_nodata(si, "%-50s %-8s %3ld :%s",
			chptr->name, modestr,
			LIST_LENGTH(&chptr->members),
			chptr->topic);
	else if(show_topicwho && show_topic)
		command_success_nodata(si, "%-50s %3ld :%s (%s)",
			chptr->name, LIST_LENGTH(&chptr->members),
			chptr->topic, chptr->topic_setter);
	else if(show_topic)
		command_success_nodata(si, "%-50s %3ld :%s",
			chptr->name, LIST_LENGTH(&chptr->members),
			chptr->topic);
	else
		command_success_nodata(si, "%-50s %3ld",
			chptr->name, LIST_LENGTH(&chptr->members));
}

static int show_channel(channel_t *chptr, struct alis_query *query)
{
        /* skip +s channels */
        if(chptr->modes & CMODE_SEC)
                return 0;

        if((int)LIST_LENGTH(&chptr->members) < query->min ||
           (query->max && (int)LIST_LENGTH(&chptr->members) > query->max))
                return 0;

        if(query->mode)
        {
                if(query->mode_dir == DIR_SET)
                {
                        if(((chptr->modes & query->mode) == 0) ||
                           (query->mode_key && chptr->key[0] == '\0') ||
                           (query->mode_limit && !chptr->limit))
                                return 0;
                }
                else if(query->mode_dir == DIR_UNSET)
                {
                        if((chptr->modes & query->mode) ||
                           (query->mode_key && chptr->key[0] != '\0') ||
                           (query->mode_limit && chptr->limit))
                                return 0;
                }
                else if(query->mode_dir == DIR_EQUAL)
                {
                        if((chptr->modes != query->mode) ||
                           (query->mode_key && chptr->key[0] == '\0') ||
                           (query->mode_limit && !chptr->limit))
                                return 0;
                }
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
	mowgli_dictionary_iteration_state_t state;
	int maxmatch = ALIS_MAX_MATCH;

	memset(&query, 0, sizeof(struct alis_query));

        query.mask = parc >= 1 ? parv[0] : "*";
	if (!parse_alis(si, parc - 1, parv + 1, &query))
		return;

	logcommand(si, CMDLOG_GET, "LIST %s", query.mask);

	command_success_nodata(si,
		"Returning maximum of %d channel names matching '\2%s\2'",
		ALIS_MAX_MATCH, query.mask);

        /* hunting for one channel.. */
        if(strchr(query.mask, '*') == NULL && strchr(query.mask, '?') == NULL)
        {
                if((chptr = channel_find(query.mask)) != NULL)
                {
                        if(!(chptr->modes & CMODE_SEC))
                                print_channel(si, chptr, &query);
                }

		command_success_nodata(si, "End of output");
                return;
        }

	MOWGLI_DICTIONARY_FOREACH(chptr, &state, chanlist)
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
	command_help(si, &alis_cmdtree);
}

static void alis_handler(sourceinfo_t *si, int parc, char *parv[])
{
        char orig[BUFSIZE];
	char *cmd;
	char *text;

        /* this should never happen */
        if (parv[0][0] == '&')
        {
                slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
                return;
        }

        /* make a copy of the original for debugging */
        strlcpy(orig, parv[parc - 1], BUFSIZE);

	/* lets go through this to get the command */
	cmd = strtok(parv[parc - 1], " ");
        text = strtok(NULL, "");

	if (!cmd)
		return;
	if (*cmd == '\001')
	{
		handle_ctcp_common(si, cmd, text);
		return;
	}

        /* take the command through the hash table */
        command_exec_split(alis, si, cmd, text, &alis_cmdtree);
}
