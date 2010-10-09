/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are documented in doc/LICENCE.
 *
 * This file contains functionality implementing OperServ CLEARCHAN.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/clearchan", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Robin Burchell <surreal.w00t@gmail.com>"
);

#define CLEAR_KICK 1
#define CLEAR_KILL 2
#define CLEAR_AKILL 3

static void os_cmd_clearchan(sourceinfo_t *si, int parc, char *parv[]);

command_t os_clearchan = { "CLEARCHAN", N_("Clears a channel via KICK, KILL or AKILL"), PRIV_CHAN_ADMIN, 3, os_cmd_clearchan, { .path = "oservice/clearchan" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_clearchan);
}

void _moddeinit(void)
{
	service_named_unbind_command("operserv", &os_clearchan);
}

static void os_cmd_clearchan(sourceinfo_t *si, int parc, char *parv[])
{
	chanuser_t *cu = NULL;
	mowgli_node_t *n, *tn;
	channel_t *c = NULL;
	int action;
	char *actionstr = parv[0];
	char *targchan = parv[1];
	char *treason = parv[2];
	char reason[512];
	int matches = 0;
	int ignores = 0;

	if (!actionstr || !targchan || !treason)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLEARCHAN");
		command_fail(si, fault_needmoreparams, _("Syntax: CLEARCHAN KICK|KILL|AKILL <#channel> <reason>"));
 		return;
	}

	c = channel_find(targchan);

	if (!c)
	{
                command_fail(si, fault_nosuch_target, _("Channel \2%s\2 does not exist."), targchan);
 		return;
	}

	/* check what they are doing is valid */
	if (!strcasecmp(actionstr, "KICK"))
		action = CLEAR_KICK;
	else if (!strcasecmp(actionstr, "KILL"))
		action = CLEAR_KILL;
	else if (!strcasecmp(actionstr, "AKILL") || !strcasecmp(actionstr, "GLINE") || !strcasecmp(actionstr, "KLINE"))
		action = CLEAR_AKILL;
	else
	{
		/* not valid! */
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid action"), actionstr);
 		return;				
	}

	if (action != CLEAR_KICK && !has_priv(si, PRIV_MASS_AKILL))
	{
		command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_MASS_AKILL);
		return;
	}

	if (action == CLEAR_KICK)
		snprintf(reason, sizeof reason, "(Clearing) %s", treason);
	else
		snprintf(reason, sizeof reason, "(Clearing %s) %s", targchan, treason);

	wallops("\2%s\2 is clearing channel %s with %s",
			get_oper_name(si), targchan, actionstr);
	command_success_nodata(si, _("Clearing \2%s\2 with \2%s\2"), targchan, actionstr);

	/* iterate over the users in channel */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, c->members.head)
	{
		cu = n->data;

		if (is_internal_client(cu->user))
			;
		else if (is_ircop(cu->user))
		{
			command_success_nodata(si, _("\2CLEARCHAN\2: Ignoring IRC Operator \2%s\2!%s@%s {%s}"), cu->user->nick, cu->user->user, cu->user->host, cu->user->gecos);
			ignores++;
		}
		else
		{
			command_success_nodata(si, _("\2CLEARCHAN\2: \2%s\2 hit \2%s\2!%s@%s {%s}"), actionstr, cu->user->nick, cu->user->user, cu->user->host, cu->user->gecos);
			matches++;

			switch (action)
			{
				case CLEAR_KICK:
					kick(si->service->me, c, cu->user, reason);
					break;
				case CLEAR_KILL:
					kill_user(si->service->me, cu->user, "%s", reason);
					break;
				case CLEAR_AKILL:
					if (is_autokline_exempt(cu->user))
						command_success_nodata(si, _("\2CLEARCHAN\2: Not klining exempt user %s!%s@%s"),
								cu->user->nick, cu->user->user, cu->user->host);
					else
						kline_sts("*", "*", cu->user->host, 604800, reason);
			}
		}
	}

	command_success_nodata(si, _("\2%d\2 matches, \2%d\2 ignores for \2%s\2 on \2%s\2"), matches, ignores, actionstr, targchan);
	logcommand(si, CMDLOG_ADMIN, "CLEARCHAN: \2%s\2 \2%s\2 (reason: \2%s\2) (\2%d\2 matches, \2%d\2 ignores)", actionstr, targchan, treason, matches, ignores);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
