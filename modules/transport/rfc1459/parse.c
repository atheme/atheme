/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * parse.c: Parsing of IRC messages.
 */

#include <atheme.h>
#include "rfc1459.h"

/* Unescapes an IRCv3 message-tags value in-place
 * \: -> ';', \s -> ' ', \\ -> '\', \r -> CR, \n -> LF
 */
static void
message_tag_unescape(char *value)
{
	char *dst = value;

	for (char *src = value; *src != '\0'; src++)
	{
		if (*src != '\\')
		{
			*dst++ = *src;
			continue;
		}

		switch (*++src)
		{
		case ':':
			*dst++ = ';';
			break;
		case 's':
			*dst++ = ' ';
			break;
		case '\\':
			*dst++ = '\\';
			break;
		case 'r':
			*dst++ = '\r';
			break;
		case 'n':
			*dst++ = '\n';
			break;
		case '\0':
			// trailing lone backslash; drop it
			*dst = '\0';
			return;
		default:
			*dst++ = *src;
			break;
		}
	}

	*dst = '\0';
}

static void
message_tag_free_cb(const char *key, void *data, void *privdata)
{
	sfree(data);
}

// parses a standard 2.8.21 style IRC stream
void
irc_parse(char *line)
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

		// If it starts with @ we have IRCv3 message tags.
		if (*line == '@')
		{
			line++;
			if (*line == ' ')
				goto cleanup; /* no tags, starts with "@ " */

			char *sp = strchr(line, ' ');
			if (!sp)
				goto cleanup; /* just "@tags" */

			*sp = '\0';
			si->tags = mowgli_patricia_create(NULL);
			for (char *tag = strtok_r(line + 1, ";", &pos); tag != NULL; tag = strtok_r(NULL, ";", &pos))
			{
				char *key = tag;
				char *value = strchr(tag, '=');
				void *old;

				if (value != NULL)
				{
					*value++ = '\0';
					message_tag_unescape(value);
				}
				else
					value = "";

				if (*key == '\0')
					continue;

				// if we get duplicate keys, keep only the latest one
				if ((old = mowgli_patricia_delete(si->tags, key)) != NULL)
					sfree(old);

				mowgli_patricia_add(si->tags, key, sstrdup(value));
			}

			line = sp + 1;
			if (*line == '\n' || *line == '\000')
				goto cleanup; /* just "@tags " */
		}

		// find the first space
		if ((pos = strchr(line, ' ')))
		{
			*pos = '\0';
			pos++;

			/* if it starts with a : we have a prefix/origin
			 * pull the origin off into `origin', and have pos for the
			 * command, message will be the part afterwards
			 */
			if (*line == ':')
			{
                        	origin = line + 1;

				si->s = server_find(origin);
				si->su = user_find(origin);

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
				if (me.recvsvr)
				{
					origin = me.actual;
					si->s = server_find(origin);
				}
				message = pos;
				command = line;
			}
		}
		else
		{
			if (me.recvsvr)
			{
				origin = me.actual;
				si->s = server_find(origin);
			}
			command = line;
			message = NULL;
		}
                if (!si->s && !si->su && me.recvsvr)
                {
                        slog(LG_DEBUG, "irc_parse(): got message from nonexistent user or server: %s", origin);
                        goto cleanup;
                }
		if (si->s == me.me)
		{
                        slog(LG_INFO, "irc_parse(): got message supposedly from myself %s: %s", si->s->name, coreLine);
                        goto cleanup;
		}
		if (si->su != NULL && si->su->server == me.me)
		{
                        slog(LG_INFO, "irc_parse(): got message supposedly from my own client %s: %s", si->su->nick, coreLine);
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

		// take the command through the hash table
		if ((pcmd = pcommand_find(command)))
		{
			if (si->su && !(pcmd->sourcetype & MSRC_USER))
			{
				slog(LG_INFO, "irc_parse(): user %s sent disallowed command %s", si->su->nick, pcmd->token);
				goto cleanup;
			}
			else if (si->s && !(pcmd->sourcetype & MSRC_SERVER))
			{
				slog(LG_INFO, "irc_parse(): server %s sent disallowed command %s", si->s->name, pcmd->token);
				goto cleanup;
			}
			else if (!me.recvsvr && !(pcmd->sourcetype & MSRC_UNREG))
			{
				slog(LG_INFO, "irc_parse(): unregistered server sent disallowed command %s", pcmd->token);
				goto cleanup;
			}
			if (parc < pcmd->minparc)
			{
				slog(LG_INFO, "irc_parse(): insufficient parameters for command %s", pcmd->token);
				goto cleanup;
			}
			handle_command(pcmd, si, parc, parv);
		}
	}

cleanup:
	if (si->tags != NULL)
	{
		mowgli_patricia_destroy(si->tags, message_tag_free_cb, NULL);
		si->tags = NULL;
	}

	atheme_object_unref(si);
}
