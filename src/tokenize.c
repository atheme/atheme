/*
 * Copyright (c) 2005 Atheme Development Group
 *
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains IRC interaction routines.
 *
 * $Id: tokenize.c 2263 2005-09-16 21:53:11Z nenolod $
 */

#include "atheme.h"

int8_t sjtoken(char *message, char delimiter, char **parv)
{
	char *next;
	uint16_t count;

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
int8_t tokenize(char *message, char **parv)
{
	char *pos = NULL;
	char *next;
	uint8_t count = 0;

	if (!message)
		return -1;

	/* first we fid out of there's a : in the message, save that string
	 * somewhere so we can set it to the last param in parv
	 * also make sure there's a space before it... if not then we're screwed
	 */
	pos = message;
	while (TRUE)
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
		if (count == 19)
		{
			/* we've reached one less than our max limit
			 * to handle the parameter we alreayd ripped off
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
