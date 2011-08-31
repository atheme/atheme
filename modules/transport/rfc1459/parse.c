/*
 * atheme-services: A collection of minimalist IRC services
 * parse.c: Parsing of IRC messages.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
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

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "rfc1459.h"

/* parses a standard 2.8.21 style IRC stream */
void irc_parse(char *line)
{
	sourceinfo_t *si;
	char *pos;
	char *origin = NULL;
	char *command = NULL;
	char *message = NULL;
	char *parv[MAXPARC + 1];
	static char coreLine[BUFSIZE];
	int parc = 0;
	unsigned int i;
	pcommand_t *pcmd;

	/* clear the parv */
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

		/* copy the original line so we know what we crashed on */
		memset((char *)&coreLine, '\0', BUFSIZE);
		mowgli_strlcpy(coreLine, line, BUFSIZE);

		slog(LG_RAWDATA, "-> %s", line);

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
                        slog(LG_DEBUG, "irc_parse(): got message from nonexistant user or server: %s", origin);
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

		/* now we should have origin (or NULL), command, and a parv
		 * with it's accompanying parc... let's make ABSOLUTELY sure
		 */
		if (!command)
		{
			slog(LG_DEBUG, "irc_parse(): command not found: %s", coreLine);
			goto cleanup;
		}

		/* take the command through the hash table */
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
			if (pcmd->handler)
			{
				pcmd->handler(si, parc, parv);
			}
		}
	}

cleanup:
	object_unref(si);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
