/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Hybrid's UID code, adopted to be as generic as possible.
 *
 * $Id: uid.c 2497 2005-10-01 04:35:25Z nenolod $
 */

#include "atheme.h"

static char new_uid[9];		/* allow for \0 */
static uint32_t uindex = 0;

void init_uid(void)
{
	uint32_t i;
	char buf[BUFSIZE];

	if (ircd->uses_p10 == TRUE)
	{
		me.numeric = sstrdup(uinttobase64(buf, (uint64_t) atoi(me.numeric), 2));
		uindex = 5;
	}
	else
		uindex = 9;


	memset(new_uid, 0, sizeof(new_uid));

	if (me.numeric != NULL)
		memcpy(new_uid, me.numeric, strlen(me.numeric));
	else
		return;

	for (i = 0; i < strlen(me.numeric); i++)
		if (new_uid[i] == '\0')
			new_uid[i] = 'A';

	for (i = strlen(me.numeric); i < uindex; i++)
		new_uid[i] = 'A';
}

char *uid_get(void)
{
	add_one_to_uid(uindex - 1);	/* index from 0 */
	return (new_uid);
}

void add_one_to_uid(uint32_t i)
{
	if (i != strlen(me.numeric))	/* Not reached server SID portion yet? */
	{
		if (new_uid[i] == 'Z')
			new_uid[i] = '0';
		else if (new_uid[i] == '9')
		{
			new_uid[i] = 'A';
			add_one_to_uid(i - 1);
		}
		else
			new_uid[i] = new_uid[i] + 1;
	}
	else
	{
		if (new_uid[i] == 'Z')
			for (i = strlen(me.numeric); i < 9; i++)
				new_uid[i] = 'A';
		else
			new_uid[i] = new_uid[i] + 1;
	}
}
