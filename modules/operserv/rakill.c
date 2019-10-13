/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006 Robin Burchell <surreal.w00t@gmail.com>
 *
 * Regexp-based AKILL implementation.
 */

/*
 * @makill regex!here.+ [akill reason]
 *  Matches `nick!user@host realname here' for each client against a given regex, and places akills.
 *  CHECK REGEX USING @RMATCH FIRST!
 */
#include <atheme.h>

static void
os_cmd_rakill(struct sourceinfo *si, int parc, char *parv[])
{
	struct atheme_regex *regex;
	char usermask[512];
	unsigned int matches = 0;
	mowgli_patricia_iteration_state_t state;
	struct user *u;
	char *args = parv[0];
	char *pattern;
	char *reason;
	struct user *source;
	int flags = 0;

	// make sure they could have done RMATCH
	if (!has_priv(si, PRIV_USER_AUSPEX))
	{
		command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_USER_AUSPEX);
		return;
	}

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RAKILL");
		command_fail(si, fault_needmoreparams, _("Syntax: RAKILL /<regex>/[i] <reason>"));
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RAKILL");
		command_fail(si, fault_badparams, _("Syntax: RAKILL /<regex>/[i] <reason>"));
		return;
	}

	reason = args;
	while (*reason == ' ')
		reason++;
	if (*reason == '\0')
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RAKILL");
		command_fail(si, fault_needmoreparams, _("Syntax: RAKILL /<regex>/[i] <reason>"));
		return;
	}

	regex = regex_create(pattern, flags);
	if (regex == NULL)
	{
		command_fail(si, fault_badparams, _("The provided regex \2%s\2 is invalid."), pattern);
		return;
	}

	source = si->su;
	// try to find them on IRC, otherwise use operserv
	if (source == NULL)
		source = si->smu != NULL && MOWGLI_LIST_LENGTH(&si->smu->logins) > 0 ?
			si->smu->logins.head->data : si->service->me;
	sprintf(usermask, "%s!%s@%s %s", source->nick, source->user, source->host, source->gecos);
	if (regex_match(regex, usermask))
	{
		regex_destroy(regex);
		command_fail(si, fault_noprivs, _("The provided regex matches you, refusing RAKILL."));
		wallops("\2%s\2 attempted to do RAKILL on \2%s\2 matching self",
				get_oper_name(si), pattern);
		logcommand(si, CMDLOG_ADMIN, "RAKILL: \2%s\2 (reason: \2%s\2) (refused, matches self)", pattern, reason);
		return;
	}

	MOWGLI_PATRICIA_FOREACH(u, &state, userlist)
	{
		sprintf(usermask, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);

		if (regex_match(regex, usermask))
		{
			// match
			command_success_nodata(si, _("\2Match:\2  %s!%s@%s %s - AKILLing"), u->nick, u->user, u->host, u->gecos);
			if (! (u->flags & UF_KLINESENT)) {
				kline_sts("*", "*", u->host, SECONDS_PER_WEEK, reason);
				u->flags |= UF_KLINESENT;
			}
			matches++;
		}
	}

	regex_destroy(regex);
	command_success_nodata(si, ngettext(N_("\2%u\2 match for \2%s\2 AKILLed."),
	                                    N_("\2%u\2 matches for \2%s\2 AKILLed."),
	                                    matches), matches, pattern);

	logcommand(si, CMDLOG_ADMIN, "RAKILL: \2%s\2 (reason: \2%s\2) (\2%u\2 matches)", pattern, reason, matches);
}

static struct command os_rakill = {
	.name           = "RAKILL",
	.desc           = N_("Sets a group of AKILLs against users matching a specific regex pattern."),
	.access         = PRIV_MASS_AKILL,
	.maxparc        = 1,
	.cmd            = &os_cmd_rakill,
	.help           = { .path = "oservice/rakill" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	service_named_bind_command("operserv", &os_rakill);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_rakill);
}

SIMPLE_DECLARE_MODULE_V1("operserv/rakill", MODULE_UNLOAD_CAPABILITY_OK)
