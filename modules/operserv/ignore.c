/*
 * Copyright (c) 2006 Patrick Fish, et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService IGNORE command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/ignore", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_ignore(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_ignore_add(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_ignore_del(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_ignore_list(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_ignore_clear(sourceinfo_t *si, int parc, char *parv[]);

command_t os_ignore = { "IGNORE", N_("Ignore a mask from services."), PRIV_ADMIN, 3, os_cmd_ignore, { .path = "oservice/ignore" } };
command_t os_ignore_add = { "ADD", N_("Add services ignore"), PRIV_ADMIN, 2, os_cmd_ignore_add, { .path = "" } };
command_t os_ignore_del = { "DEL", N_("Delete services ignore"), PRIV_ADMIN, 1, os_cmd_ignore_del, { .path = "" } };
command_t os_ignore_list = { "LIST", N_("List services ignores"), PRIV_ADMIN, 0, os_cmd_ignore_list, { .path = "" } };
command_t os_ignore_clear = { "CLEAR", N_("Clear all services ignores"), PRIV_ADMIN, 0, os_cmd_ignore_clear, { .path = "" } };

mowgli_patricia_t *os_ignore_cmds;
mowgli_list_t svs_ignore_list;


void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_ignore);

	os_ignore_cmds = mowgli_patricia_create(strcasecanon);

	/* Sub-commands */
	command_add(&os_ignore_add, os_ignore_cmds);
	command_add(&os_ignore_del, os_ignore_cmds);
	command_add(&os_ignore_clear, os_ignore_cmds);
	command_add(&os_ignore_list, os_ignore_cmds);

	use_svsignore++;
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_ignore);

	/* Sub-commands */
	command_delete(&os_ignore_add, os_ignore_cmds);
	command_delete(&os_ignore_del, os_ignore_cmds);
	command_delete(&os_ignore_list, os_ignore_cmds);
	command_delete(&os_ignore_clear, os_ignore_cmds);

	use_svsignore--;

	mowgli_patricia_destroy(os_ignore_cmds, NULL, NULL);
}

static void os_cmd_ignore(sourceinfo_t *si, int parc, char *parv[])
{
	char *cmd = parv[0];
        command_t *c;

	if (!cmd)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");
		command_fail(si, fault_needmoreparams, _("Syntax: IGNORE ADD|DEL|LIST|CLEAR <mask>"));
		return;
	}

        c = command_find(os_ignore_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		return;
	}

	command_exec(si->service, si, c, parc - 1, parv + 1);

}

static void os_cmd_ignore_add(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;
        char *target = parv[0];
	char *reason = parv[1];
	svsignore_t *svsignore;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");
		command_fail(si, fault_needmoreparams, _("Syntax: IGNORE ADD <mask> <reason>"));
		return;
	}

	if (!validhostmask(target))
	{
		command_fail(si, fault_badparams, _("Invalid host mask, %s"), target);
		return;
	}

	/* Are we already ignoring this mask? */
	MOWGLI_ITER_FOREACH(n, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		/* We're here */
		if (!strcasecmp(svsignore->mask, target))
		{
			command_fail(si, fault_nochange, _("The mask \2%s\2 already exists on the services ignore list."), svsignore->mask);
			return;
		}
	}

	svsignore = svsignore_add(target, reason);
	svsignore->setby = sstrdup(get_storage_oper_name(si));
	svsignore->settime = CURRTIME;

	command_success_nodata(si, _("\2%s\2 has been added to the services ignore list."), target);

	logcommand(si, CMDLOG_ADMIN, "IGNORE:ADD: \2%s\2 (reason: \2%s\2)", target, reason);
	wallops("%s added a services ignore for \2%s\2 (%s).", get_oper_name(si), target, reason);

	return;
}

static void os_cmd_ignore_del(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	mowgli_node_t *n, *tn;
	svsignore_t *svsignore;

	if (target == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");
		command_fail(si, fault_needmoreparams, _("Syntax: IGNORE ADD|DEL|LIST|CLEAR <mask>"));
		return;
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		if (!strcasecmp(svsignore->mask,target))
		{
			command_success_nodata(si, _("\2%s\2 has been removed from the services ignore list."), svsignore->mask);

			svsignore_delete(svsignore);

			wallops("%s removed \2%s\2 from the services ignore list.", get_oper_name(si), target);
			logcommand(si, CMDLOG_ADMIN, "IGNORE:DEL: \2%s\2", target);

			return;
		}
	}

	command_fail(si, fault_nosuch_target, _("\2%s\2 was not found on the services ignore list."), target);
	return;
}

static void os_cmd_ignore_clear(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n, *tn;
	svsignore_t *svsignore;

	if (MOWGLI_LIST_LENGTH(&svs_ignore_list) == 0)
	{
		command_success_nodata(si, _("Services ignore list is empty."));
		return;
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		command_success_nodata(si, _("\2%s\2 has been removed from the services ignore list."), svsignore->mask);
		mowgli_node_delete(n,&svs_ignore_list);
		mowgli_node_free(n);
		free(svsignore->mask);
		free(svsignore->setby);
		free(svsignore->reason);

	}

	command_success_nodata(si, _("Services ignore list has been wiped!"));

	wallops("\2%s\2 wiped the services ignore list.", get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "IGNORE:CLEAR");

	return;
}


static void os_cmd_ignore_list(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	unsigned int i = 1;
	svsignore_t *svsignore;
	char strfbuf[32];
	struct tm tm;

	if (MOWGLI_LIST_LENGTH(&svs_ignore_list) == 0)
	{
		command_success_nodata(si, _("The services ignore list is empty."));
		return;
	}

	command_success_nodata(si, _("Current Ignore list entries:"));
	command_success_nodata(si, "-------------------------");

	MOWGLI_ITER_FOREACH(n, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		tm = *localtime(&svsignore->settime);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		command_success_nodata(si, _("%d: %s by %s on %s (Reason: %s)"), i, svsignore->mask, svsignore->setby, strfbuf, svsignore->reason);
		i++;
	}

	command_success_nodata(si, "-------------------------");

	logcommand(si, CMDLOG_ADMIN, "IGNORE:LIST");

	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
