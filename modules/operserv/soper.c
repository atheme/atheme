/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Dynamic services operator privileges
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/soper", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_soper(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_soper_list(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_soper_listclass(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_soper_add(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_soper_del(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_soper_setpass(sourceinfo_t *si, int parc, char *parv[]);

command_t os_soper = { "SOPER", N_("Shows and changes services operator privileges."), AC_NONE, 3, os_cmd_soper, { .path = "oservice/soper" } };

command_t os_soper_list = { "LIST", N_("Lists services operators."), PRIV_VIEWPRIVS, 0, os_cmd_soper_list, { .path = "" } };
command_t os_soper_listclass = { "LISTCLASS", N_("Lists operclasses."), PRIV_VIEWPRIVS, 0, os_cmd_soper_listclass, { .path = "" } };
command_t os_soper_add = { "ADD", N_("Grants services operator privileges to an account."), PRIV_GRANT, 2, os_cmd_soper_add, { .path = "" } };
command_t os_soper_del = { "DEL", N_("Removes services operator privileges from an account."), PRIV_GRANT, 1, os_cmd_soper_del, { .path = "" } };
command_t os_soper_setpass = { "SETPASS", N_("Changes a password for services operator privileges."), PRIV_GRANT, 2, os_cmd_soper_setpass, { .path = "" } };

mowgli_patricia_t *os_soper_cmds;

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_soper);

	os_soper_cmds = mowgli_patricia_create(strcasecanon);

	command_add(&os_soper_list, os_soper_cmds);
	command_add(&os_soper_listclass, os_soper_cmds);
	command_add(&os_soper_add, os_soper_cmds);
	command_add(&os_soper_del, os_soper_cmds);
	command_add(&os_soper_setpass, os_soper_cmds);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_soper);
	command_delete(&os_soper_list, os_soper_cmds);
	command_delete(&os_soper_listclass, os_soper_cmds);
	command_delete(&os_soper_add, os_soper_cmds);
	command_delete(&os_soper_del, os_soper_cmds);

	mowgli_patricia_destroy(os_soper_cmds, NULL, NULL);
}

static void os_cmd_soper(sourceinfo_t *si, int parc, char *parv[])
{
	command_t *c;

	if (!has_any_privs(si))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SOPER");
		command_fail(si, fault_needmoreparams, _("Syntax: SOPER LIST|LISTCLASS|ADD|DEL [account] [operclass]"));
		return;
	}

	c = command_find(os_soper_cmds, parv[0]);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		return;
	}

	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void os_cmd_soper_list(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	soper_t *soper;
	const char *typestr;

	logcommand(si, CMDLOG_GET, "SOPER:LIST");
	command_success_nodata(si, "%-20s %-5s %-20s", _("Account"), _("Type"), _("Operclass"));
	command_success_nodata(si, "%-20s %-5s %-20s", "--------------------", "-----", "--------------------");
	MOWGLI_ITER_FOREACH(n, soperlist.head)
	{
		soper = n->data;
		if (!(soper->flags & SOPER_CONF))
			typestr = "DB";
		else if (soper->myuser != NULL)
			typestr = "Conf";
		else
			typestr = "Conf*";
		command_success_nodata(si, "%-20s %-5s %-20s",
				soper->myuser != NULL ? entity(soper->myuser)->name : soper->name,
				typestr,
				soper->classname);
	}
	command_success_nodata(si, "%-20s %-5s %-20s", "--------------------", "-----", "--------------------");
	command_success_nodata(si, _("End of services operator list"));
}

static void os_cmd_soper_listclass(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	operclass_t *operclass;

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

static void os_cmd_soper_add(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	operclass_t *operclass;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SOPER ADD");
		command_fail(si, fault_needmoreparams, _("Syntax: SOPER ADD <account> <operclass>"));
		return;
	}

	mu = myuser_find_ext(parv[0]);
	if (mu == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), parv[0]);
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

	wallops("\2%s\2 is changing oper class for \2%s\2 to \2%s\2",
		get_oper_name(si), entity(mu)->name, operclass->name);
	logcommand(si, CMDLOG_ADMIN, "SOPER:ADD: \2%s\2 \2%s\2", entity(mu)->name, operclass->name);
	if (is_soper(mu))
		soper_delete(mu->soper);
	soper_add(entity(mu)->name, operclass->name, 0, NULL);
	command_success_nodata(si, _("Set class for \2%s\2 to \2%s\2."), entity(mu)->name, operclass->name);
}

static void os_cmd_soper_del(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SOPER DEL");
		command_fail(si, fault_needmoreparams, _("Syntax: SOPER DEL <account>"));
		return;
	}

	mu = myuser_find_ext(parv[0]);
	if (mu == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), parv[0]);
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

static void os_cmd_soper_setpass(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	mowgli_node_t *n;
	user_t *u;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SOPER SETPASS");
		command_fail(si, fault_needmoreparams, _("Syntax: SOPER SETPASS <account> [password]"));
		return;
	}

	mu = myuser_find_ext(parv[0]);
	if (mu == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), parv[0]);
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
		if (mu->soper->password)
			free(mu->soper->password);
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
		if (mu->soper->password)
			free(mu->soper->password);
		mu->soper->password = NULL;
		command_success_nodata(si, _("Cleared password for \2%s\2."), entity(mu)->name);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
