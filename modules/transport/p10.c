/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2011 Atheme Project (http://atheme.org/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * parse.c: Parsing of IRC messages.
 */

#include <atheme.h>

// parses a P10 IRC stream
static void
p10_parse(char *line)
{
	struct sourceinfo *si;
	char *pos;
	char *origin = NULL;
	char *command = NULL;
	char *message = NULL;
	char *parv[MAXPARC + 1];
	static char coreLine[BUFSIZE];
	int parc = 0;
	unsigned int i;
	struct proto_cmd *pcmd;

	// clear the parv
	for (i = 0; i <= MAXPARC; i++)
		parv[i] = NULL;

	si = sourceinfo_create();
	si->connection = curr_uplink->conn;
	si->output_limit = MAX_IRC_OUTPUT_LINES;

	if (line != NULL)
	{
		/* sometimes we'll get a blank line with just a \n on it...
		 * catch those here... they'll core us later on if we don't
		 */
		if (*line == '\n')
			goto cleanup;
		if (*line == '\000')
			goto cleanup;

		// copy the original line so we know what we crashed on
		memset((char *)&coreLine, '\0', BUFSIZE);
		mowgli_strlcpy(coreLine, line, BUFSIZE);

		slog(LG_RAWDATA, "-> %s", line);

		// find the first space
		if ((pos = strchr(line, ' ')))
		{
			*pos = '\0';
			pos++;

			/* if it starts with a : we have a prefix/origin
			 * pull the origin off into `origin', and have pos for the
			 * command, message will be the part afterwards
			 */
			if (*line == ':' || me.recvsvr)
			{
				origin = line;
				if (*origin == ':')
				{
					origin++;
                                	si->s = server_find(origin);
                                	si->su = user_find_named(origin);
				}
				else
				{
                                	si->s = server_find(origin);
                                	si->su = user_find(origin);
				}

				if ((message = strchr(pos, ' ')))
				{
					*message = '\0';
					message++;
					command = pos;
				}
				else
				{
					command = pos;
					message = NULL;
				}
			}
			else
			{
				message = pos;
				command = line;
			}
		}

                if (!si->s && !si->su && me.recvsvr)
                {
                        slog(LG_DEBUG, "p10_parse(): got message from nonexistent user or server: %s", origin);
                        goto cleanup;
                }
		if (si->s == me.me)
		{
                        slog(LG_INFO, "p10_parse(): got message supposedly from myself %s: %s", si->s->name, coreLine);
                        goto cleanup;
		}
		if (si->su != NULL && si->su->server == me.me)
		{
                        slog(LG_INFO, "p10_parse(): got message supposedly from my own client %s: %s", si->su->nick, coreLine);
                        goto cleanup;
		}
		si->smu = si->su != NULL ? si->su->myuser : NULL;

		/* okay, the nasty part is over, now we need to make a
		 * parv out of what's left
		 */

		if (message)
		{
			if (*message == ':')
			{
				message++;
				parv[0] = message;
				parc = 1;
			}
			else
				parc = tokenize(message, parv);
		}
		else
			parc = 0;

		/* now we should have origin (or NULL), command, and a parv
		 * with it's accompanying parc... let's make ABSOLUTELY sure
		 */
		if (!command)
		{
			slog(LG_DEBUG, "p10_parse(): command not found: %s", coreLine);
			goto cleanup;
		}

		// take the command through the hash table
		if ((pcmd = pcommand_find(command)))
		{
			if (si->su && !(pcmd->sourcetype & MSRC_USER))
			{
				slog(LG_INFO, "p10_parse(): user %s sent disallowed command %s", si->su->nick, pcmd->token);
				goto cleanup;
			}
			else if (si->s && !(pcmd->sourcetype & MSRC_SERVER))
			{
				slog(LG_INFO, "p10_parse(): server %s sent disallowed command %s", si->s->name, pcmd->token);
				goto cleanup;
			}
			else if (!me.recvsvr && !(pcmd->sourcetype & MSRC_UNREG))
			{
				slog(LG_INFO, "p10_parse(): unregistered server sent disallowed command %s", pcmd->token);
				goto cleanup;
			}
			if (parc < pcmd->minparc)
			{
				slog(LG_INFO, "p10_parse(): insufficient parameters for command %s", pcmd->token);
				goto cleanup;
			}
			if (pcmd->handler)
			{
				pcmd->handler(si, parc, parv);
			}
		}
	}

cleanup:
	atheme_object_unref(si);
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "transport/rfc1459")

	parse = &p10_parse;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("transport/p10", MODULE_UNLOAD_CAPABILITY_NEVER)
