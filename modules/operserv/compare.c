/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are documented in doc/LICENCE.
 *
 * This file contains functionality implementing OperServ COMPARE.
 *
 * $Id: compare.c 6199 2006-08-21 09:44:29Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/compare", FALSE, _modinit, _moddeinit,
	"$Id: compare.c 6199 2006-08-21 09:44:29Z jilles $",
	"Robin Burchell <surreal.w00t@gmail.com>"
);

static void os_cmd_compare(char *origin);

command_t os_compare = { "COMPARE", "Compares two users or channels.", 
		PRIV_CHAN_AUSPEX, os_cmd_compare };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_compare, os_cmdtree);
	help_addentry(os_helptree, "COMPARE", "help/oservice/compare", NULL);
}

void _moddeinit(void)
{
	command_delete(&os_compare, os_cmdtree);
	help_delentry(os_helptree, "COMPARE");
}

static void os_cmd_compare(char *origin)
{
	char *object1 = strtok(NULL, " ");
	char *object2 = strtok(NULL, "");
	channel_t *c1, *c2;
	user_t *u1, *u2;
	node_t *n1, *n2;
	chanuser_t *cu1, *cu2;
	int matches = 0;

	int temp = 0;
	char buf[512];
	char tmpbuf[100];

	bzero(buf, 512);
	bzero(tmpbuf, 100);

	if (!object1 || !object2)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "COMPARE");
		notice(opersvs.nick, origin, "Syntax: COMPARE <nick|#channel> <nick|#channel>");
		return;
	}

	if (*object1 == '#')
	{
		if (*object2 == '#')
		{
			/* compare on two channels */
			c1 = channel_find(object1);
			c2 = channel_find(object2);

			if (!c1 || !c2)
			{
				notice(opersvs.nick, origin, "Both channels must exist for @compare");
				return;				
			}

			notice(opersvs.nick, origin, "Common users in \2%s\2 and \2%s\2", object1, object2);

			/* iterate over the users in channel 1 */
			LIST_FOREACH(n1, c1->members.head)
			{
				cu1 = n1->data;

				/* now iterate over each of channel 2's members, and compare them to the current user from ch1 */
				LIST_FOREACH(n2, c2->members.head)
				{
					cu2 = n2->data;

					/* match! */
					if (cu1->user == cu2->user)
					{
						/* common user! */
						snprintf(tmpbuf, 99, "%s, ", cu1->user->nick);
						strcat((char *)buf, tmpbuf);
						bzero(tmpbuf, 100);

						/* if too many, output to user */
						if (temp >= 5 || strlen(buf) > 300)
						{
							notice(opersvs.nick, origin, "%s", buf);
							bzero(buf, 512);
							temp = 0;
						}

						temp++;
						matches++;
					}
				}
			}
		}
		else
		{
			/* bad syntax */
			notice(opersvs.nick, origin, "Bad syntax for @compare. Use @compare on two channels, or two users.");
			return;				
		}
	}
	else
	{
		if (*object2 == '#')
		{
			/* bad syntax */
			notice(opersvs.nick, origin, "Bad syntax for @compare. Use @compare on two channels, or two users.");
			return;				
		}
		else
		{
			/* compare on two users */
			u1 = user_find_named(object1);
			u2 = user_find_named(object2);

			if (!u1 || !u2)
			{
				notice(opersvs.nick, origin, "Both users must exist for @compare");
				return;				
			}

			notice(opersvs.nick, origin, "Common channels for \2%s\2 and \2%s\2", object1, object2);

			/* iterate over the channels of user 1 */
			LIST_FOREACH(n1, u1->channels.head)
			{
				cu1 = n1->data;

				/* now iterate over each of user 2's channels to the current channel from user 1 */
				LIST_FOREACH(n2, u2->channels.head)
				{
					cu2 = n2->data;

					/* match! */
					if (cu1->chan == cu2->chan)
					{
						/* common channel! */
						snprintf(tmpbuf, 99, "%s, ", cu1->chan->name);
						strcat((char *)buf, tmpbuf);
						bzero(tmpbuf, 100);

						/* if too many, output to user */
						if (temp >= 5 || strlen(buf) > 300)
						{
							notice(opersvs.nick, origin, "%s", buf);
							bzero(buf, 512);
							temp = 0;
						}

						temp++;
						matches++;
					}
				}
			}
		}
	}

	if (buf[0] != 0)
		notice(opersvs.nick, origin, "%s", buf);

	notice(opersvs.nick, origin, "\2%d\2 matches comparing %s and %s", matches, object1, object2);
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "COMPARE %s to %s (%d matches)", object1, object2, matches);
	snoop("COMPARE: \2%s\2 to \2%s\2 by \2%s\2", object1, object2, origin);
}
