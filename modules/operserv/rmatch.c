/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Regex usersearch feature.
 */

/*
 * @match regex!here.+
 *  Matches `nick!user@host realname here' for each client against a given regex, and dumps matches.
 */
#include "atheme.h"
#include <limits.h>

#define MAXMATCHES_DEF 1000

static void
os_cmd_rmatch(struct sourceinfo *si, int parc, char *parv[])
{
	struct atheme_regex *regex;
	char usermask[512];
	unsigned int matches = 0, maxmatches;
	mowgli_patricia_iteration_state_t state;
	struct user *u;
	char *args = parv[0];
	char *pattern;
	int flags = 0;

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RMATCH");
		command_fail(si, fault_needmoreparams, _("Syntax: RMATCH /<regex>/[i] [FORCE]"));
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RMATCH");
		command_fail(si, fault_badparams, _("Syntax: RMATCH /<regex>/[i] [FORCE]"));
		return;
	}

	while (*args == ' ')
		args++;

	if (!strcasecmp(args, "FORCE"))
		maxmatches = UINT_MAX;
	else if (*args == '\0')
		maxmatches = MAXMATCHES_DEF;
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RMATCH");
		command_fail(si, fault_badparams, _("Syntax: RMATCH /<regex>/[i] [FORCE]"));
		return;
	}

	regex = regex_create(pattern, flags);

	if (regex == NULL)
	{
		command_fail(si, fault_badparams, _("The provided regex \2%s\2 is invalid."), pattern);
		return;
	}

	MOWGLI_PATRICIA_FOREACH(u, &state, userlist)
	{
		sprintf(usermask, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);

		if (regex_match(regex, usermask))
		{
			matches++;
			if (matches <= maxmatches)
				command_success_nodata(si, _("\2Match:\2  %s!%s@%s %s"), u->nick, u->user, u->host, u->gecos);
			else if (matches == maxmatches + 1)
			{
				command_success_nodata(si, _("Too many matches, not displaying any more"));
				command_success_nodata(si, _("Add the FORCE keyword to see them all"));
			}
		}
	}

	regex_destroy(regex);
	command_success_nodata(si, _("\2%d\2 matches for %s"), matches, pattern);
	logcommand(si, CMDLOG_ADMIN, "RMATCH: \2%s\2 (\2%d\2 matches)", pattern, matches);
}

static struct command os_rmatch = { "RMATCH", N_("Scans the network for users based on a specific regex pattern."), PRIV_USER_AUSPEX, 1, os_cmd_rmatch, { .path = "oservice/rmatch" } };

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("operserv", &os_rmatch);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_rmatch);
}

SIMPLE_DECLARE_MODULE_V1("operserv/rmatch", MODULE_UNLOAD_CAPABILITY_OK)
