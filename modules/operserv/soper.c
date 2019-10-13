/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006 Atheme Project (http://atheme.org/)
 *
 * Dynamic services operator privileges
 */

#include <atheme.h>

static mowgli_patricia_t *os_soper_cmds = NULL;

static void
os_cmd_soper(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! has_any_privs(si))
	{
		(void) command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SOPER");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SOPER LIST|LISTCLASS|ADD|DEL [account] [operclass]"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, os_soper_cmds, "SOPER");
}

static void
os_cmd_soper_list(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	struct soper *soper;
	const char *typestr;

	logcommand(si, CMDLOG_GET, "SOPER:LIST");

	/* TRANSLATORS: Adjust these numbers only if the translated column
	 * headers would exceed that length. Pay particular attention to
	 * also changing the numbers in the format string inside the loop
	 * below to match them, and beware that these format strings are
	 * shared across multiple files!
	 */
	command_success_nodata(si, _("%-20s %-8s %-20s"), _("Account"), _("Type"), _("Operclass"));
	command_success_nodata(si, "--------------------------------------------------");

	MOWGLI_ITER_FOREACH(n, soperlist.head)
	{
		soper = n->data;
		if (!(soper->flags & SOPER_CONF))
			typestr = "DB";
		else if (soper->myuser != NULL)
			typestr = "Conf";
		else
			typestr = "Conf*";
		command_success_nodata(si, _("%-20s %-8s %-20s"),
				soper->myuser != NULL ? entity(soper->myuser)->name : soper->name,
				typestr,
				soper->classname);
	}

	command_success_nodata(si, "--------------------------------------------------");
	command_success_nodata(si, _("End of services operator list."));
}

static void
os_cmd_soper_listclass(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	struct operclass *operclass;

	logcommand(si, CMDLOG_GET, "SOPER:LISTCLASS");
	command_success_nodata(si, _("Oper class list:"));
	MOWGLI_ITER_FOREACH(n, operclasslist.head)
	{
		operclass = n->data;
		command_success_nodata(si, "%c%c %s",
				has_all_operclass(si, operclass) ? '+' : '-',
				operclass->flags & OPERCLASS_NEEDOPER ? '*' : ' ',
				operclass->name);
	}
	command_success_nodata(si, _("End of oper class list"));
}

static void
os_cmd_soper_add(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	struct operclass *operclass;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SOPER ADD");
		command_fail(si, fault_needmoreparams, _("Syntax: SOPER ADD <account> <operclass> [password]"));
		return;
	}

	mu = myuser_find_ext(parv[0]);
	if (mu == NULL)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (is_conf_soper(mu))
	{
		command_fail(si, fault_noprivs, _("You may not modify \2%s\2's operclass as it is defined in the configuration file."), entity(mu)->name);
		return;
	}

	operclass = operclass_find(parv[1]);
	if (operclass == NULL)
	{
		command_fail(si, fault_nosuch_target, _("No such oper class \2%s\2."), parv[1]);
		return;
	}
	else if (mu->soper != NULL && mu->soper->operclass == operclass)
	{
		command_fail(si, fault_nochange, _("Oper class for \2%s\2 is already set to \2%s\2."), entity(mu)->name, operclass->name);
		return;
	}

	const char *hash = NULL;

	if (!has_all_operclass(si, operclass))
	{
		command_fail(si, fault_noprivs, _("Oper class \2%s\2 has more privileges than you."), operclass->name);
		return;
	}
	else if (mu->soper != NULL && mu->soper->operclass != NULL && !has_all_operclass(si, mu->soper->operclass))
	{
		command_fail(si, fault_noprivs, _("Oper class for \2%s\2 is set to \2%s\2 which you are not authorized to change."),
				entity(mu)->name, mu->soper->operclass->name);
		return;
	}
	else if (parv[2] && !(hash = crypt_password(parv[2])))
	{
		command_fail(si, fault_internalerror, _("Hash generation for oper password failed."));
		return;
	}

	wallops("\2%s\2 is changing oper class for \2%s\2 to \2%s\2",
		get_oper_name(si), entity(mu)->name, operclass->name);

	logcommand(si, CMDLOG_ADMIN, "SOPER:ADD: \2%s\2 \2%s\2", entity(mu)->name, operclass->name);

	if (is_soper(mu))
		soper_delete(mu->soper);

	soper_add(entity(mu)->name, operclass->name, 0, hash);

	command_success_nodata(si, _("Set class for \2%s\2 to \2%s\2."), entity(mu)->name, operclass->name);
}

static void
os_cmd_soper_del(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SOPER DEL");
		command_fail(si, fault_needmoreparams, _("Syntax: SOPER DEL <account>"));
		return;
	}

	mu = myuser_find_ext(parv[0]);
	if (mu == NULL)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (is_conf_soper(mu))
	{
		command_fail(si, fault_noprivs, _("You may not modify \2%s\2's operclass as it is defined in the configuration file."), entity(mu)->name);
		return;
	}

	if (!is_soper(mu))
	{
		command_fail(si, fault_nochange, _("\2%s\2 does not have an operclass set."), entity(mu)->name);
		return;
	}
	else if (mu->soper->operclass != NULL && !has_all_operclass(si, mu->soper->operclass))
	{
		command_fail(si, fault_noprivs, _("Oper class for \2%s\2 is set to \2%s\2 which you are not authorized to change."),
				entity(mu)->name, mu->soper->operclass->name);
		return;
	}

	wallops("\2%s\2 is removing oper class for \2%s\2",
		get_oper_name(si), entity(mu)->name);
	logcommand(si, CMDLOG_ADMIN, "SOPER:DELETE: \2%s\2", entity(mu)->name);
	soper_delete(mu->soper);
	command_success_nodata(si, _("Removed class for \2%s\2."), entity(mu)->name);
}

static void
os_cmd_soper_setpass(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	mowgli_node_t *n;
	struct user *u;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SOPER SETPASS");
		command_fail(si, fault_needmoreparams, _("Syntax: SOPER SETPASS <account> [password]"));
		return;
	}

	mu = myuser_find_ext(parv[0]);
	if (mu == NULL)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (is_conf_soper(mu))
	{
		command_fail(si, fault_noprivs, _("You may not modify \2%s\2's operclass as it is defined in the configuration file."), entity(mu)->name);
		return;
	}

	if (!is_soper(mu))
	{
		command_fail(si, fault_nochange, _("\2%s\2 does not have an operclass set."), entity(mu)->name);
		return;
	}

	if (mu->soper->operclass != NULL && !has_all_operclass(si, mu->soper->operclass))
	{
		command_fail(si, fault_noprivs, _("Oper class for \2%s\2 is set to \2%s\2 which you are not authorized to change."),
				entity(mu)->name, mu->soper->operclass->name);
		return;
	}

	if (parc >= 2)
	{
		if (mu->soper->password == NULL &&
				!command_find(si->service->commands, "IDENTIFY"))
		{
			command_fail(si, fault_noprivs, _("Refusing to set a services operator password if %s IDENTIFY is not loaded."), si->service->nick);
			return;
		}
		wallops("\2%s\2 is changing services operator password for \2%s\2",
				get_oper_name(si), entity(mu)->name);
		logcommand(si, CMDLOG_ADMIN, "SOPER:SETPASS: \2%s\2 (set)", entity(mu)->name);
		sfree(mu->soper->password);
		mu->soper->password = sstrdup(parv[1]);
		command_success_nodata(si, _("Set password for \2%s\2 to \2%s\2."), entity(mu)->name, parv[1]);
		MOWGLI_ITER_FOREACH(n, mu->logins.head)
		{
			u = n->data;
			if (u->flags & UF_SOPER_PASS)
			{
				u->flags &= ~UF_SOPER_PASS;
				notice(si->service->nick, u->nick, "You are no longer identified to %s.", si->service->nick);
			}
		}
	}
	else
	{
		wallops("\2%s\2 is clearing services operator password for \2%s\2",
				get_oper_name(si), entity(mu)->name);
		logcommand(si, CMDLOG_ADMIN, "SOPER:SETPASS: \2%s\2 (clear)", entity(mu)->name);
		sfree(mu->soper->password);
		mu->soper->password = NULL;
		command_success_nodata(si, _("Cleared password for \2%s\2."), entity(mu)->name);
	}
}

static struct command os_soper = {
	.name           = "SOPER",
	.desc           = N_("Shows and changes services operator privileges."),
	.access         = AC_NONE,
	.maxparc        = 4,
	.cmd            = &os_cmd_soper,
	.help           = { .path = "oservice/soper" },
};

static struct command os_soper_list = {
	.name           = "LIST",
	.desc           = N_("Lists services operators."),
	.access         = PRIV_VIEWPRIVS,
	.maxparc        = 0,
	.cmd            = &os_cmd_soper_list,
	.help           = { .path = "" },
};

static struct command os_soper_listclass = {
	.name           = "LISTCLASS",
	.desc           = N_("Lists operclasses."),
	.access         = PRIV_VIEWPRIVS,
	.maxparc        = 0,
	.cmd            = &os_cmd_soper_listclass,
	.help           = { .path = "" },
};

static struct command os_soper_add = {
	.name           = "ADD",
	.desc           = N_("Grants services operator privileges to an account."),
	.access         = PRIV_GRANT,
	.maxparc        = 3,
	.cmd            = &os_cmd_soper_add,
	.help           = { .path = "" },
};

static struct command os_soper_del = {
	.name           = "DEL",
	.desc           = N_("Removes services operator privileges from an account."),
	.access         = PRIV_GRANT,
	.maxparc        = 1,
	.cmd            = &os_cmd_soper_del,
	.help           = { .path = "" },
};

static struct command os_soper_setpass = {
	.name           = "SETPASS",
	.desc           = N_("Changes a password for services operator privileges."),
	.access         = PRIV_GRANT,
	.maxparc        = 2,
	.cmd            = &os_cmd_soper_setpass,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	if (! (os_soper_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&os_soper_list, os_soper_cmds);
	(void) command_add(&os_soper_listclass, os_soper_cmds);
	(void) command_add(&os_soper_add, os_soper_cmds);
	(void) command_add(&os_soper_del, os_soper_cmds);
	(void) command_add(&os_soper_setpass, os_soper_cmds);

	(void) service_named_bind_command("operserv", &os_soper);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("operserv", &os_soper);

	(void) mowgli_patricia_destroy(os_soper_cmds, &command_delete_trie_cb, os_soper_cmds);
}

SIMPLE_DECLARE_MODULE_V1("operserv/soper", MODULE_UNLOAD_CAPABILITY_OK)
