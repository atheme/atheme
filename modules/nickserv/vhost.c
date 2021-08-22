/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 *
 * Allows setting a vhost on an account
 */

#include <atheme.h>

static void
do_sethost(struct user *u, stringref host)
{
	if (!strcmp(u->vhost, host))
		return;

	user_sethost(nicksvs.me->me, u, host);
}

static void
do_sethost_all(struct myuser *mu, stringref host)
{
	mowgli_node_t *n;
	struct user *u;

	MOWGLI_ITER_FOREACH(n, mu->logins.head)
	{
		u = n->data;

		do_sethost(u, host ? host : u->host);
	}
}

// VHOST <account> [host]  (legacy)
// VHOST <account> ON|OFF [host]
static void
ns_cmd_vhost(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *host;
	struct myuser *mu;
	struct metadata *md, *markmd;
	bool force = false, ismarked = false;
	char cmdtext[NICKLEN + HOSTLEN + 20];
	char timestring[16];
	struct hook_user_needforce needforce_hdata;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "VHOST");
		command_fail(si, fault_needmoreparams, _("Syntax: VHOST <account> ON|OFF [vhost]"));
		return;
	}

	// find the user...
	if (!(mu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
		return;
	}

	if (parc == 1)
		host = NULL;
	else if (parc == 2 && (strchr(parv[1], '.') || strchr(parv[1], ':') ||
					strchr(parv[1], '/')))
		host = parv[1];
	else if (!strcasecmp(parv[1], "ON"))
	{
		if (parc < 3)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "VHOST");
			command_fail(si, fault_needmoreparams, _("Syntax: VHOST <account> ON <vhost> [FORCE]"));
			return;
		}
		host = parv[2];
		if (parc == 4 && !strcasecmp(parv[3], "FORCE"))
			force = true;
		else if (parc >= 4)
		{
			command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "VHOST");
			command_fail(si, fault_needmoreparams, _("Syntax: VHOST <account> ON <vhost> [FORCE]"));
			return;
		}
	}
	else if (!strcasecmp(parv[1], "OFF"))
	{
		host = NULL;
		if (parc == 3 && !strcasecmp(parv[2], "FORCE"))
			force = true;
		else if (parc >= 3)
		{
			command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "VHOST");
			command_fail(si, fault_needmoreparams, _("Syntax: VHOST <account> OFF [FORCE]"));
			return;
		}
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "VHOST");
		command_fail(si, fault_badparams, _("Syntax: VHOST <account> ON|OFF [vhost]"));
		return;
	}

	if ((markmd = metadata_find(mu, "private:mark:setter")))
	{
		ismarked = true;

		if (!force)
		{
			logcommand(si, CMDLOG_ADMIN, "failed VHOST \2%s\2 (marked by \2%s\2)", entity(mu)->name, markmd->value);
			command_fail(si, fault_badparams, _("This operation cannot be performed on %s, because the account has been marked by %s."), entity(mu)->name, markmd->value);
			if (has_priv(si, PRIV_MARK))
			{
				if (host)
					snprintf(cmdtext, sizeof cmdtext,
							"VHOST %s ON %s FORCE",
							entity(mu)->name, host);
				else
					snprintf(cmdtext, sizeof cmdtext,
							"VHOST %s OFF FORCE",
							entity(mu)->name);
				command_fail(si, fault_badparams, _("Use %s to override this restriction."), cmdtext);
			}
			return;
		}
		else if (!has_priv(si, PRIV_MARK))
		{
			logcommand(si, CMDLOG_ADMIN, "failed VHOST \2%s\2 (marked by \2%s\2)", entity(mu)->name, markmd->value);
			command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_MARK);
			return;
		}
	}

	needforce_hdata.si = si;
	needforce_hdata.mu = mu;
	needforce_hdata.allowed = 1;

	hook_call_user_needforce(&needforce_hdata);

	if (!needforce_hdata.allowed)
	{
		ismarked = true;

		if (!force)
		{
			logcommand(si, CMDLOG_ADMIN, "failed VHOST \2%s\2 (marked)", entity(mu)->name);
			command_fail(si, fault_badparams, _("This operation cannot be performed on %s, because the account has been marked."), entity(mu)->name);
			if (has_priv(si, PRIV_MARK))
			{
				if (host)
					snprintf(cmdtext, sizeof cmdtext,
							"VHOST %s ON %s FORCE",
							entity(mu)->name, host);
				else
					snprintf(cmdtext, sizeof cmdtext,
							"VHOST %s OFF FORCE",
							entity(mu)->name);
				command_fail(si, fault_badparams, _("Use %s to override this restriction."), cmdtext);
			}
			return;
		}
		else if (!has_priv(si, PRIV_MARK))
		{
			logcommand(si, CMDLOG_ADMIN, "failed VHOST \2%s\2 (marked)", entity(mu)->name);
			command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_MARK);
			return;
		}
	}

	// deletion action
	if (!host)
	{
		if (!metadata_find(mu, "private:usercloak"))
		{
			command_fail(si, fault_nochange, _("\2%s\2 does not have a vhost set."), entity(mu)->name);
			return;
		}
		snprintf(timestring, 16, "%lu", (unsigned long)time(NULL));
		metadata_delete(mu, "private:usercloak");
		metadata_add(mu, "private:usercloak-timestamp", timestring);
		metadata_add(mu, "private:usercloak-assigner", get_source_name(si));

		command_success_nodata(si, _("Deleted vhost for \2%s\2."), entity(mu)->name);
		logcommand(si, CMDLOG_ADMIN, "VHOST:REMOVE: \2%s\2", entity(mu)->name);
		if (ismarked)
		{
			wallops("\2%s\2 deleted vhost from the \2MARKED\2 account %s.", get_oper_name(si), entity(mu)->name);
			if (markmd) {
				command_success_nodata(si, _("Overriding MARK placed by %s on the account %s."), markmd->value, entity(mu)->name);
			} else {
				command_success_nodata(si, _("Overriding MARK(s) placed on the account %s."), entity(mu)->name);
			}
		}
		do_sethost_all(mu, NULL); // restore user vhost from user host
		return;
	}

	if (!check_vhost_validity(si, host))
		return;

	md = metadata_find(mu, "private:usercloak");
	if (md != NULL && !strcmp(md->value, host))
	{
		command_fail(si, fault_nochange, _("\2%s\2 already has the given vhost set."), entity(mu)->name);
		return;
	}

	snprintf(timestring, 16, "%lu", (unsigned long)time(NULL));

	metadata_add(mu, "private:usercloak", host);
	metadata_add(mu, "private:usercloak-timestamp", timestring);
	metadata_add(mu, "private:usercloak-assigner", get_source_name(si));

	command_success_nodata(si, _("Assigned vhost \2%s\2 to \2%s\2."),
			host, entity(mu)->name);
	logcommand(si, CMDLOG_ADMIN, "VHOST:ASSIGN: \2%s\2 to \2%s\2",
			host, entity(mu)->name);
	if (ismarked)
	{
		wallops("\2%s\2 set vhost %s on the \2MARKED\2 account %s.", get_oper_name(si), host, entity(mu)->name);
		if (markmd) {
			command_success_nodata(si, _("Overriding MARK placed by %s on the account %s."), markmd->value, entity(mu)->name);
		} else {
			command_success_nodata(si, _("Overriding MARK(s) placed on the account %s."), entity(mu)->name);
		}
	}
	do_sethost_all(mu, host);
	return;
}

static void
ns_cmd_listvhost(struct sourceinfo *si, int parc, char *parv[])
{
	const char *pattern;
	struct myentity_iteration_state state;
	struct myentity *mt;
	struct myuser *mu;
	struct metadata *md;
	unsigned int matches = 0;

	pattern = parc >= 1 ? parv[0] : "*";

	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		mu = user(mt);
		md = metadata_find(mu, "private:usercloak");
		if (md == NULL)
			continue;
		if (!match(pattern, md->value))
		{
			command_success_nodata(si, "- %-30s %s", entity(mu)->name, md->value);
			matches++;
		}
	}

	logcommand(si, CMDLOG_ADMIN, "LISTVHOST: \2%s\2 (\2%u\2 matches)", pattern, matches);
	if (matches == 0)
		command_success_nodata(si, _("No vhosts matched pattern \2%s\2"), pattern);
	else
		command_success_nodata(si, ngettext(N_("\2%u\2 match for pattern \2%s\2"),
						    N_("\2%u\2 matches for pattern \2%s\2"), matches), matches, pattern);
}

static void
vhost_on_identify(struct user *u)
{
	struct myuser *mu = u->myuser;
	struct metadata *md;

	// NO CLOAK?!*$*%*&&$(!&
	if (!(md = metadata_find(mu, "private:usercloak")))
		return;

	do_sethost(u, md->value);
}

static struct command ns_vhost = {
	.name           = "VHOST",
	.desc           = N_("Manages user virtualhosts."),
	.access         = PRIV_USER_VHOST,
	.maxparc        = 4,
	.cmd            = &ns_cmd_vhost,
	.help           = { .path = "nickserv/vhost" },
};

static struct command ns_listvhost = {
	.name           = "LISTVHOST",
	.desc           = N_("Lists user virtualhosts."),
	.access         = PRIV_USER_AUSPEX,
	.maxparc        = 1,
	.cmd            = &ns_cmd_listvhost,
	.help           = { .path = "nickserv/listvhost" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	hook_add_first_user_identify(vhost_on_identify);
	service_named_bind_command("nickserv", &ns_vhost);
	service_named_bind_command("nickserv", &ns_listvhost);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	hook_del_user_identify(vhost_on_identify);
	service_named_unbind_command("nickserv", &ns_vhost);
	service_named_unbind_command("nickserv", &ns_listvhost);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/vhost", MODULE_UNLOAD_CAPABILITY_OK)
