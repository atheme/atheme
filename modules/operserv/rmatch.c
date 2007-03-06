/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Regex usersearch feature.
 *
 * $Id: rmatch.c 7895 2007-03-06 02:40:03Z pippijn $
 */

/*
 * @match regex!here.+
 *  Matches `nick!user@host realname here' for each client against a given regex, and dumps matches.
 */
#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/rmatch", FALSE, _modinit, _moddeinit,
	"$Id: rmatch.c 7895 2007-03-06 02:40:03Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *os_cmdtree;
list_t *os_helptree;

static void os_cmd_rmatch(sourceinfo_t *si, int parc, char *parv[]);

command_t os_rmatch = { "RMATCH", N_("Scans the network for users based on a specific regex pattern."), PRIV_USER_AUSPEX, 1, os_cmd_rmatch };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_rmatch, os_cmdtree);
	help_addentry(os_helptree, "RMATCH", "help/oservice/rmatch", NULL);
}

void _moddeinit(void)
{
	command_delete(&os_rmatch, os_cmdtree);
	help_delentry(os_helptree, "RMATCH");
}

static void os_cmd_rmatch(sourceinfo_t *si, int parc, char *parv[])
{
	regex_t *regex;
	char usermask[512];
	uint32_t matches = 0;
	dictionary_iteration_state_t state;
	user_t *u;
	char *args = parv[0];
	char *pattern;
	int flags = 0;

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RMATCH");
		command_fail(si, fault_needmoreparams, _("Syntax: RMATCH /<regex>/[i]"));
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RMATCH");
		command_fail(si, fault_badparams, _("Syntax: RMATCH /<regex>/[i]"));
		return;
	}

	regex = regex_create(pattern, flags);
	
	if (regex == NULL)
	{
		command_fail(si, fault_badparams, _("The provided regex \2%s\2 is invalid."), pattern);
		return;
	}
		
	DICTIONARY_FOREACH(u, &state, userlist)
	{
		sprintf(usermask, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);

		if (regex_match(regex, usermask) == TRUE)
		{
			/* match */
			command_success_nodata(si, _("\2Match:\2  %s!%s@%s %s"), u->nick, u->user, u->host, u->gecos);
			matches++;
		}
	}
	
	regex_destroy(regex);
	command_success_nodata(si, _("\2%d\2 matches for %s"), matches, pattern);
	logcommand(si, CMDLOG_ADMIN, "RMATCH %s (%d matches)", pattern, matches);
	snoop("RMATCH: \2%s\2 by \2%s\2", pattern, get_oper_name(si));
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
