/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006 Robin Burchell <surreal.w00t@gmail.com>
 *
 * This file contains functionality implementing OperServ COMPARE.
 */

#include <atheme.h>

static void
os_cmd_compare(struct sourceinfo *si, int parc, char *parv[])
{
	char *object1 = parv[0];
	char *object2 = parv[1];
	struct channel *c1, *c2;
	struct user *u1, *u2;
	mowgli_node_t *n1, *n2;
	struct chanuser *cu1, *cu2;
	unsigned int matches = 0;

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
			// compare on two channels
			c1 = channel_find(object1);
			c2 = channel_find(object2);

			if (!c1 || !c2)
			{
				command_fail(si, fault_nosuch_target, _("Both channels must exist for @compare"));
				return;
			}

			command_success_nodata(si, _("Common users in \2%s\2 and \2%s\2"), object1, object2);

			// iterate over the users in channel 1
			MOWGLI_ITER_FOREACH(n1, c1->members.head)
			{
				cu1 = n1->data;

				// now iterate over each of channel 2's members, and compare them to the current user from ch1
				MOWGLI_ITER_FOREACH(n2, c2->members.head)
				{
					cu2 = n2->data;

					// match!
					if (cu1->user == cu2->user)
					{
						// common user!
						snprintf(tmpbuf, 99, "%s, ", cu1->user->nick);
						strcat((char *)buf, tmpbuf);
						memset(tmpbuf, '\0', 100);

						// if too many, output to user
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
			// bad syntax
			command_fail(si, fault_badparams, _("Bad syntax for @compare. Use @compare on two channels, or two users."));
			return;
		}
	}
	else
	{
		if (*object2 == '#')
		{
			// bad syntax
			command_fail(si, fault_badparams, _("Bad syntax for @compare. Use @compare on two channels, or two users."));
			return;
		}
		else
		{
			// compare on two users
			u1 = user_find_named(object1);
			u2 = user_find_named(object2);

			if (!u1 || !u2)
			{
				command_fail(si, fault_nosuch_target, _("Both users must exist for @compare"));
				return;
			}

			command_success_nodata(si, _("Common channels for \2%s\2 and \2%s\2"), object1, object2);

			// iterate over the channels of user 1
			MOWGLI_ITER_FOREACH(n1, u1->channels.head)
			{
				cu1 = n1->data;

				// now iterate over each of user 2's channels to the current channel from user 1
				MOWGLI_ITER_FOREACH(n2, u2->channels.head)
				{
					cu2 = n2->data;

					// match!
					if (cu1->chan == cu2->chan)
					{
						// common channel!
						snprintf(tmpbuf, 99, "%s, ", cu1->chan->name);
						strcat((char *)buf, tmpbuf);
						memset(tmpbuf, '\0', 100);

						// if too many, output to user
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

	command_success_nodata(si, ngettext(N_("\2%u\2 match comparing \2%s\2 and \2%s\2."),
	                                    N_("\2%u\2 matches comparing \2%s\2 and \2%s\2."),
	                                    matches), matches, object1, object2);

	logcommand(si, CMDLOG_ADMIN, "COMPARE: \2%s\2 to \2%s\2 (\2%u\2 matches)", object1, object2, matches);
}

static struct command os_compare = {
	.name           = "COMPARE",
	.desc           = N_("Compares two users or channels."),
	.access         = PRIV_CHAN_AUSPEX,
	.maxparc        = 2,
	.cmd            = &os_cmd_compare,
	.help           = { .path = "oservice/compare" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	service_named_bind_command("operserv", &os_compare);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_compare);
}

SIMPLE_DECLARE_MODULE_V1("operserv/compare", MODULE_UNLOAD_CAPABILITY_OK)
