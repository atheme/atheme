/*
 * atheme-services: A collection of minimalist IRC services
 * tokenize.c: Tokenization routines.
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
#include "pmodule.h"

int sjtoken(char *message, char delimiter, char **parv)
{
	char *next;
	unsigned int count;

	if (!message)
		return 0;

	/* now we take the beginning of the message and find all the spaces...
	 * set them to \0 and use 'next' to go through the string
	 */
	next = message;

	/* eat any additional delimiters */
	while (*next == delimiter)
		next++;

	parv[0] = next;
	count = 1;

	while (*next)
	{
		/* this is fine here, since we don't have a :delimited
		 * parameter like tokenize
		 */

		if (count == 256)
		{
			/* we've reached our limit */
			slog(LG_DEBUG, "sjtokenize(): reached param limit");
			return count;
		}

		if (*next == delimiter)
		{
			*next = '\0';
			next++;
			/* eat any additional delimiters */
			while (*next == delimiter)
				next++;
			/* if it's the end of the string, it's simply
			 ** an extra space at the end.  here we break.
			 */
			if (*next == '\0')
				break;

			/* if it happens to be a stray \r, break too */
			if (*next == '\r')
				break;

			parv[count] = next;
			count++;
		}
		else
			next++;
	}

	return count;
}

/* this splits apart a message with origin and command picked off already */
int tokenize(char *message, char **parv)
{
	char *pos = NULL;
	char *next;
	unsigned int count = 0;

	if (!message)
		return -1;

	/* first we fid out of there's a : in the message, save that string
	 * somewhere so we can set it to the last param in parv
	 * also make sure there's a space before it... if not then we're screwed
	 */
	pos = message;
	while (true)
	{
		if ((pos = strchr(pos, ':')))
		{
			pos--;
			if (*pos != ' ')
			{
				pos += 2;
				continue;
			}
			*pos = '\0';
			pos++;
			*pos = '\0';
			pos++;
			break;
		}
		else
			break;
	}

	/* now we take the beginning of the message and find all the spaces...
	 * set them to \0 and use 'next' to go through the string
	 */

	next = message;
	parv[0] = message;
	count = 1;

	while (*next)
	{
		if (count == MAXPARC)
		{
			/* we've reached one less than our max limit
			 * to handle the parameter we already ripped off
			 */
			slog(LG_DEBUG, "tokenize(): reached para limit");
			return count;
		}
		if (*next == ' ')
		{
			*next = '\0';
			next++;
			/* eat any additional spaces */
			while (*next == ' ')
				next++;
			/* if it's the end of the string, it's simply
			 * an extra space before the :parameter. break.
			 */
			if (*next == '\0')
				break;
			parv[count] = next;
			count++;
		}
		else
			next++;
	}

	if (pos)
	{
		parv[count] = pos;
		count++;
	}

	return count;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
