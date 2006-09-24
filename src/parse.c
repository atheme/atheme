/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains IRC interaction routines.
 *
 * $Id: parse.c 6437 2006-09-24 13:18:06Z jilles $
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"

/* by default, we want the 2.8.21 parser */
void (*parse) (char *line) = &irc_parse;

/* parses a standard 2.8.21 style IRC stream */
void irc_parse(char *line)
{
	sourceinfo_t si = { 0 };
	char *pos;
	char *origin = NULL;
	char *command = NULL;
	char *message = NULL;
	char *parv[20];
	static char coreLine[BUFSIZE];
	int parc = 0;
	uint8_t i;
	pcommand_t *pcmd;

	/* clear the parv */
	for (i = 0; i < 20; i++)
		parv[i] = NULL;

	if (line != NULL)
	{
		/* sometimes we'll get a blank line with just a \n on it...
		 * catch those here... they'll core us later on if we don't
		 */
		if (*line == '\n')
			return;
		if (*line == '\000')
			return;

		/* copy the original line so we know what we crashed on */
		memset((char *)&coreLine, '\0', BUFSIZE);
		strlcpy(coreLine, line, BUFSIZE);

		slog(LG_DEBUG, "-> %s", line);

		/* find the first space */
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

				si.s = server_find(origin);
				si.su = user_find(origin);

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
					si.s = server_find(origin);
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
				si.s = server_find(origin);
			}
			command = line;
			message = NULL;
		}
                if (!si.s && !si.su && me.recvsvr)
                {
                        slog(LG_INFO, "irc_parse(): got message from nonexistant user or server: %s", origin);
                        return;
                }
		si.smu = si.su != NULL ? si.su->myuser : NULL;

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
			slog(LG_DEBUG, "irc_parse(): command not found: %s", coreLine);
			return;
		}

		/* take the command through the hash table */
		if ((pcmd = pcommand_find(command)))
		{
			if (si.su && !(pcmd->sourcetype & MSRC_USER))
			{
				slog(LG_INFO, "irc_parse(): user %s sent disallowed command %s", si.su->nick, pcmd->token);
				return;
			}
			else if (si.s && !(pcmd->sourcetype & MSRC_SERVER))
			{
				slog(LG_INFO, "irc_parse(): server %s sent disallowed command %s", si.s->name, pcmd->token);
				return;
			}
			else if (!me.recvsvr && !(pcmd->sourcetype & MSRC_UNREG))
			{
				slog(LG_INFO, "irc_parse(): unregistered server sent disallowed command %s", pcmd->token);
				return;
			}
			if (parc < pcmd->minparc)
			{
				slog(LG_INFO, "irc_parse(): insufficient parameters for command %s", pcmd->token);
				return;
			}
			if (pcmd->handler)
			{
				pcmd->handler(&si, parc, parv);
			}
		}
	}
}

/* parses a P10 IRC stream */
void p10_parse(char *line)
{
	sourceinfo_t si = { 0 };
	char *pos;
	char *origin = NULL;
	char *command = NULL;
	char *message = NULL;
	char *parv[20];
	static char coreLine[BUFSIZE];
	int parc = 0;
	uint8_t i;
	pcommand_t *pcmd;

	/* clear the parv */
	for (i = 0; i < 20; i++)
		parv[i] = NULL;

	if (line != NULL)
	{
		/* sometimes we'll get a blank line with just a \n on it...
		 * catch those here... they'll core us later on if we don't
		 */
		if (*line == '\n')
			return;
		if (*line == '\000')
			return;

		/* copy the original line so we know what we crashed on */
		memset((char *)&coreLine, '\0', BUFSIZE);
		strlcpy(coreLine, line, BUFSIZE);

		slog(LG_DEBUG, "-> %s", line);

		/* find the first spcae */
		if ((pos = strchr(line, ' ')))
		{
			*pos = '\0';
			pos++;
			/* if it starts with a : we have a prefix/origin
			 * pull the origin off into `origin', and have pos for the
			 * command, message will be the part afterwards
			 */
			if (*line == ':' || me.recvsvr == TRUE)
			{
				origin = line;
				if (*origin == ':')
				{
					origin++;
                                	si.s = server_find(origin);
                                	si.su = user_find_named(origin);
				}
				else
				{
                                	si.s = server_find(origin);
                                	si.su = user_find(origin);
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

                if (!si.s && !si.su && me.recvsvr)
                {
                        slog(LG_DEBUG, "irc_parse(): got message from nonexistant user or server: %s", origin);
                        return;
                }
		si.smu = si.su != NULL ? si.su->myuser : NULL;

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
			slog(LG_DEBUG, "irc_parse(): command not found: %s", coreLine);
			return;
		}

		/* take the command through the hash table */
		if ((pcmd = pcommand_find(command)))
		{
			if (si.su && !(pcmd->sourcetype & MSRC_USER))
			{
				slog(LG_INFO, "irc_parse(): user %s sent disallowed command %s", si.su->nick, pcmd->token);
				return;
			}
			else if (si.s && !(pcmd->sourcetype & MSRC_SERVER))
			{
				slog(LG_INFO, "irc_parse(): server %s sent disallowed command %s", si.s->name, pcmd->token);
				return;
			}
			else if (!me.recvsvr && !(pcmd->sourcetype & MSRC_UNREG))
			{
				slog(LG_INFO, "irc_parse(): unregistered server sent disallowed command %s", pcmd->token);
				return;
			}
			if (parc < pcmd->minparc)
			{
				slog(LG_INFO, "irc_parse(): insufficient parameters for command %s", pcmd->token);
				return;
			}
			if (pcmd->handler)
			{
				pcmd->handler(&si, parc, parv);
				return;
			}
		}
	}
}
