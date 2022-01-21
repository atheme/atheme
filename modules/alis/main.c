/*
 * SPDX-License-Identifier: (ISC AND BSD-3-Clause)
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 * SPDX-URL: https://spdx.org/licenses/BSD-3-Clause.html
 *
 * Copyright (C) 2005-2007 William Pitcock, et al.
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 *
 * ALIS, based on the ratbox-services implementation.
 */

/*
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
 */

#include <atheme.h>

#define ALIS_MAXMATCH_MIN       8U
#define ALIS_MAXMATCH_DEF       64U
#define ALIS_MAXMATCH_MAX       128U

enum alis_mode_cmp
{
	MODECMP_NONE            = 0,
	MODECMP_EQUAL           = 1,
	MODECMP_UNSET           = 2,
	MODECMP_SET             = 3,
};

struct alis_query
{
	unsigned int            min;
	unsigned int            max;
	unsigned int            skip;
	unsigned int            match_limit;
	unsigned int            mode;
	enum alis_mode_cmp      mode_cmp;
	bool                    mode_key;
	bool                    mode_limit;
	bool                    mode_ext[256];
	bool                    show_mode;
	bool                    show_topicwho;
	bool                    show_secret;
	char                    mask[BUFSIZE];
	char                    topic[BUFSIZE];
};

static struct service *alissvs = NULL;
static unsigned int alis_max_matches = ALIS_MAXMATCH_DEF;

static bool
alis_parse_mode(struct sourceinfo *const restrict si, const char *restrict arg,
                struct alis_query *const restrict query)
{
	static const char opt[] = "-mode";

	if (! arg[0] || ! arg[1])
	{
		(void) command_fail(si, fault_badparams, _("Invalid option for \2%s\2"), opt);
		return false;
	}
	switch (*arg++)
	{
		case '+':
			query->mode_cmp = MODECMP_SET;
			break;

		case '-':
			query->mode_cmp = MODECMP_UNSET;
			break;

		case '=':
			query->mode_cmp = MODECMP_EQUAL;
			break;

		default:
			(void) command_fail(si, fault_badparams, _("Invalid option for \2%s\2"), opt);
			return false;
	}
	while (*arg)
	{
		switch (*arg)
		{
			case 'k':
				query->mode_key = true;
				break;

			case 'l':
				query->mode_limit = true;
				break;

			default:
			{
				const unsigned int mf = mode_to_flag(*arg);

				if ((! mf) || ((ircd->oper_only_modes & mf) && (! has_priv(si, PRIV_CHAN_AUSPEX))))
				{
					(void) command_fail(si, fault_badparams, _("Invalid option for \2%s\2"), opt);
					return false;
				}

				query->mode |= mf;

				for (size_t i = 0; ignore_mode_list[i].mode; i++)
					if (*arg == ignore_mode_list[i].mode)
						query->mode_ext[i] = true;

				break;
			}
		}

		arg++;
	}

	return true;
}

static bool
alis_parse_query(struct sourceinfo *const restrict si, const int parc, char **const restrict parv,
                 struct alis_query *const restrict query)
{
	const char *arg = parv[0];
	const char *opt = NULL;
	size_t i = 1;

	(void) memset(query, 0x00, sizeof *query);

	if (! arg)
		(void) mowgli_strlcpy(query->mask, "*", sizeof query->mask);
	else if (! VALID_GLOBAL_CHANNEL_PFX(arg) && ! strchr(arg, '*') && ! strchr(arg, '?'))
		(void) snprintf(query->mask, sizeof query->mask, "*%s*", arg);
	else
		(void) mowgli_strlcpy(query->mask, arg, sizeof query->mask);

	while (parc && (opt = parv[i++]))
	{
		if (strcasecmp(opt, "-min") == 0)
		{
			if (! (arg = parv[i++]) || ! string_to_uint(arg, &query->min) || ! query->min)
			{
				(void) command_fail(si, fault_badparams, _("Invalid option for \2%s\2"), opt);
				return false;
			}
		}
		else if (strcasecmp(opt, "-max") == 0)
		{
			if (! (arg = parv[i++]) || ! string_to_uint(arg, &query->max) || ! query->max)
			{
				(void) command_fail(si, fault_badparams, _("Invalid option for \2%s\2"), opt);
				return false;
			}
		}
		else if (strcasecmp(opt, "-maxmatches") == 0)
		{
			if (! (arg = parv[i++]) || ! string_to_uint(arg, &query->match_limit) || ! query->match_limit)
			{
				(void) command_fail(si, fault_badparams, _("Invalid option for \2%s\2"), opt);
				return false;
			}

			if (si->su && ! has_priv(si, PRIV_CHAN_AUSPEX) && query->match_limit > alis_max_matches)
				query->match_limit = alis_max_matches;
		}
		else if (strcasecmp(opt, "-skip") == 0)
		{
			if (! (arg = parv[i++]) || ! string_to_uint(arg, &query->skip) || ! query->skip)
			{
				(void) command_fail(si, fault_badparams, _("Invalid option for \2%s\2"), opt);
				return false;
			}
		}
		else if (strcasecmp(opt, "-topic") == 0)
		{
			if (! (arg = parv[i++]))
			{
				(void) command_fail(si, fault_badparams, _("Invalid option for \2%s\2"), opt);
				return false;
			}

			if (strchr(arg, '*'))
				(void) mowgli_strlcpy(query->topic, arg, sizeof query->topic);
			else
				(void) snprintf(query->topic, sizeof query->topic, "*%s*", arg);
		}
		else if (strcasecmp(opt, "-show") == 0)
		{
			if (! (arg = parv[i++]))
			{
				(void) command_fail(si, fault_badparams, _("Invalid option for \2%s\2"), opt);
				return false;
			}

			if (strchr(arg, 'm'))
				query->show_mode = true;

			if (strchr(arg, 't'))
				query->show_topicwho = true;
		}
		else if (strcasecmp(opt, "-mode") == 0)
		{
			if (! (arg = parv[i++]))
			{
				(void) command_fail(si, fault_badparams, _("Invalid option for \2%s\2"), opt);
				return false;
			}

			if (! alis_parse_mode(si, arg, query))
				return false;
		}
		else if (strcasecmp(opt, "-showsecret") == 0)
		{
			if (! has_priv(si, PRIV_CHAN_AUSPEX))
			{
				(void) command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
				return false;
			}

			query->show_secret = true;
		}
		else
		{
			(void) command_fail(si, fault_badparams, _("Invalid option \2%s\2"), opt);
			return false;
		}
	}

	if (! query->match_limit)
		query->match_limit = alis_max_matches;

	return true;
}

static void
alis_print_channel(struct sourceinfo *const restrict si, const struct alis_query *const restrict query,
                   const struct channel *const restrict chptr)
{
	char buf[BUFSIZE];
	char tmp[BUFSIZE];
	char topic[BUFSIZE];
	bool show_topic = true;

	if (chptr->topic && *chptr->topic)
	{
		(void) mowgli_strlcpy(topic, chptr->topic, sizeof topic);
		(void) strip_ctrl(topic);
	}
	else
		show_topic = false;

	(void) memset(buf, 0x00, sizeof buf);

	(void) snprintf(tmp, sizeof tmp, "%-48s ", chptr->name);
	(void) mowgli_strlcat(buf, tmp, sizeof buf);

	if (query->show_mode)
	{
		(void) snprintf(tmp, sizeof tmp, "%-8s ", channel_modes(chptr, false));
		(void) mowgli_strlcat(buf, tmp, sizeof buf);
	}

	(void) snprintf(tmp, sizeof tmp, "%4zu ", MOWGLI_LIST_LENGTH(&chptr->members));
	(void) mowgli_strlcat(buf, tmp, sizeof buf);

	if (show_topic)
	{
		(void) snprintf(tmp, sizeof tmp, ":%s ", topic);
		(void) mowgli_strlcat(buf, tmp, sizeof buf);
	}

	if (show_topic && query->show_topicwho)
	{
		(void) snprintf(tmp, sizeof tmp, "(%s)", chptr->topic_setter);
		(void) mowgli_strlcat(buf, tmp, sizeof buf);
	}

//	(void) command_success_string(si, chptr->name, "%s", buf);
	(void) command_success_nodata(si, "%s", buf);
}

static bool
alis_show_channel(const struct alis_query *const restrict query, const struct channel *const restrict chptr)
{
	if ((chptr->modes & CMODE_SEC) && ! query->show_secret)
		return false;
	if (query->min && MOWGLI_LIST_LENGTH(&chptr->members) < query->min)
		return false;
	if (query->max && MOWGLI_LIST_LENGTH(&chptr->members) > query->max)
		return false;

	if (query->mode_cmp == MODECMP_SET)
	{
		if ((chptr->modes & query->mode) != query->mode)
			return false;
		if (query->mode_key && ! chptr->key)
			return false;
		if (query->mode_limit && ! chptr->limit)
			return false;
		for (size_t i = 0; ignore_mode_list[i].mode; i++)
			if (query->mode_ext[i] && ! chptr->extmodes[i])
				return false;
	}
	else if (query->mode_cmp == MODECMP_UNSET)
	{
		if (chptr->modes & query->mode)
			return false;
		if (query->mode_key && chptr->key)
			return false;
		if (query->mode_limit && chptr->limit)
			return false;
		for (size_t i = 0; ignore_mode_list[i].mode; i++)
			if (query->mode_ext[i] && chptr->extmodes[i])
				return false;
	}
	else if (query->mode_cmp == MODECMP_EQUAL)
	{
		if ((chptr->modes & ~(CMODE_KEY | CMODE_LIMIT)) != query->mode)
			return false;
		if (query->mode_key && ! chptr->key)
			return false;
		if (query->mode_limit && ! chptr->limit)
			return false;
		for (size_t i = 0; ignore_mode_list[i].mode; i++)
			if (query->mode_ext[i] && ! chptr->extmodes[i])
				return false;
	}

	if (*query->mask && match(query->mask, chptr->name))
		return false;

	if (*query->topic && match(query->topic, chptr->topic))
		return false;

	return true;
}

static void
alis_cmd_list_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	struct alis_query query;

	if (! alis_parse_query(si, parc, parv, &query))
		// This function logs error messages on failure
		return;

	(void) command_success_nodata(si, _("Returning maximum of \2%u\2 channel names matching '\2%s\2'"),
	                                    query.match_limit, query.mask);

	// Hunting for one channel ?
	if (! strchr(query.mask, '*') && ! strchr(query.mask, '?'))
	{
		struct channel *const chptr = channel_find(query.mask);

		if (chptr && (! (chptr->modes & CMODE_SEC) || (si->su && chanuser_find(chptr, si->su))))
			(void) alis_print_channel(si, &query, chptr);

		goto end;
	}

	struct channel *chptr;
	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(chptr, &state, chanlist)
	{
		if (! alis_show_channel(&query, chptr))
			continue;

		if (query.skip)
		{
			query.skip--;
			continue;
		}

		(void) alis_print_channel(si, &query, chptr);

		if (--query.match_limit)
			continue;

		(void) command_success_nodata(si, _("Maximum channel output reached"));
		break;
	}

end:
	(void) command_success_nodata(si, _("End of output."));

	if (query.show_secret)
		(void) logcommand(si, CMDLOG_ADMIN, "LIST: \2%s\2 (SHOWSECRET)", query.mask);
	else
		(void) logcommand(si, CMDLOG_GET, "LIST: \2%s\2", query.mask);
}

static void
alis_cmd_help_func(struct sourceinfo *si, int parc, char *parv[])
{
	if (parv[0])
	{
		(void) help_display(si, alissvs, parv[0], alissvs->commands);
		return;
	}

	(void) help_display_prefix(si, alissvs);

	(void) command_success_nodata(si, _("\2%s\2 allows searching for channels with more\n"
	                                    "flexibility than the \2/LIST\2 command."), alissvs->nick);

	(void) help_display_newline(si);
	(void) command_help(si, alissvs->commands);
	(void) help_display_moreinfo(si, alissvs, NULL);
	(void) help_display_locations(si);
	(void) help_display_suffix(si);
}

static struct command alis_cmd_list = {
	.name           = "LIST",
	.desc           = N_("Lists channels matching given parameters."),
	.access         = AC_NONE,
	.maxparc        = 10,
	.cmd            = &alis_cmd_list_func,
	.help           = { .path = "alis/list" },
};

static struct command alis_cmd_help = {
	.name           = "HELP",
	.desc           = STR_HELP_DESCRIPTION,
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &alis_cmd_help_func,
	.help           = { .path = "help" },
};

static void
mod_init(struct module *const restrict m)
{
	if (! (alissvs = service_add("alis", NULL)))
	{
		(void) slog(LG_ERROR, "%s: service_add() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) add_uint_conf_item("MAXMATCHES", &alissvs->conf_table, 0, &alis_max_matches,
	                          ALIS_MAXMATCH_MIN, ALIS_MAXMATCH_MAX, ALIS_MAXMATCH_DEF);

	(void) service_bind_command(alissvs, &alis_cmd_list);
	(void) service_bind_command(alissvs, &alis_cmd_help);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) del_conf_item("MAXMATCHES", &alissvs->conf_table);
	(void) service_delete(alissvs);
}

SIMPLE_DECLARE_MODULE_V1("alis/main", MODULE_UNLOAD_CAPABILITY_OK)
