/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are documented in doc/LICENCE.
 *
 * This file contains functionality implementing OperServ COMPARE.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/compare", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Robin Burchell <surreal.w00t@gmail.com>"
);

static void os_cmd_compare(sourceinfo_t *si, int parc, char *parv[]);

command_t os_compare = { "COMPARE", N_("Compares two users or channels."), PRIV_CHAN_AUSPEX, 2, os_cmd_compare, { .path = "oservice/compare" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_compare);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &os_compare);
}

static void os_cmd_compare(sourceinfo_t *si, int parc, char *parv[])
{
	char *object1 = parv[0];
	char *object2 = parv[1];
	channel_t *c1, *c2;
	user_t *u1, *u2;
	mowgli_node_t *n1, *n2;
	chanuser_t *cu1, *cu2;
	int matches = 0;

	int temp = 0;
	char buf[512];
	char tmpbuf[100];

	memset(buf, '\0', 512);
	memset(tmpbuf, '\0', 100);

	if (!object1 || !object2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "COMPARE");
		command_fail(si, fault_needmoreparams, _("Syntax: COMPARE <nick|#channel> <nick|#channel>"));
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
				command_fail(si, fault_nosuch_target, _("Both channels must exist for @compare"));
				return;				
			}

			command_success_nodata(si, _("Common users in \2%s\2 and \2%s\2"), object1, object2);

			/* iterate over the users in channel 1 */
			MOWGLI_ITER_FOREACH(n1, c1->members.head)
			{
				cu1 = n1->data;

				/* now iterate over each of channel 2's members, and compare them to the current user from ch1 */
				MOWGLI_ITER_FOREACH(n2, c2->members.head)
				{
					cu2 = n2->data;

					/* match! */
					if (cu1->user == cu2->user)
					{
						/* common user! */
						snprintf(tmpbuf, 99, "%s, ", cu1->user->nick);
						strcat((char *)buf, tmpbuf);
						memset(tmpbuf, '\0', 100);

						/* if too many, output to user */
						if (temp >= 5 || strlen(buf) > 300)
						{
							command_success_nodata(si, "%s", buf);
							memset(buf, '\0', 512);
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
			command_fail(si, fault_badparams, _("Bad syntax for @compare. Use @compare on two channels, or two users."));
			return;				
		}
	}
	else
	{
		if (*object2 == '#')
		{
			/* bad syntax */
			command_fail(si, fault_badparams, _("Bad syntax for @compare. Use @compare on two channels, or two users."));
			return;				
		}
		else
		{
			/* compare on two users */
			u1 = user_find_named(object1);
			u2 = user_find_named(object2);

			if (!u1 || !u2)
			{
				command_fail(si, fault_nosuch_target, _("Both users must exist for @compare"));
				return;				
			}

			command_success_nodata(si, _("Common channels for \2%s\2 and \2%s\2"), object1, object2);

			/* iterate over the channels of user 1 */
			MOWGLI_ITER_FOREACH(n1, u1->channels.head)
			{
				cu1 = n1->data;

				/* now iterate over each of user 2's channels to the current channel from user 1 */
				MOWGLI_ITER_FOREACH(n2, u2->channels.head)
				{
					cu2 = n2->data;

					/* match! */
					if (cu1->chan == cu2->chan)
					{
						/* common channel! */
						snprintf(tmpbuf, 99, "%s, ", cu1->chan->name);
						strcat((char *)buf, tmpbuf);
						memset(tmpbuf, '\0', 100);

						/* if too many, output to user */
						if (temp >= 5 || strlen(buf) > 300)
						{
							command_success_nodata(si, "%s", buf);
							memset(buf, '\0', 512);
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
		command_success_nodata(si, "%s", buf);

	command_success_nodata(si, _("\2%d\2 matches comparing %s and %s"), matches, object1, object2);
	logcommand(si, CMDLOG_ADMIN, "COMPARE: \2%s\2 to \2%s\2 (\2%d\2 matches)", object1, object2, matches);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
