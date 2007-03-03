/*
 * Copyright (c) 2006 Patrick Fish, et al
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService IGNORE command.
 *
 * $Id: ignore.c 7779 2007-03-03 13:55:42Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/ignore", FALSE, _modinit, _moddeinit,
	"$Id: ignore.c 7779 2007-03-03 13:55:42Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_ignore(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_ignore_add(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_ignore_del(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_ignore_list(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_ignore_clear(sourceinfo_t *si, int parc, char *parv[]);

command_t os_ignore = { "IGNORE", "Ignore a mask from services.", PRIV_ADMIN, 3, os_cmd_ignore };
command_t os_ignore_add = { "ADD", "Add services ignore", PRIV_ADMIN, 2, os_cmd_ignore_add };
command_t os_ignore_del = { "DEL", "Delete services ignore", PRIV_ADMIN, 1, os_cmd_ignore_del };
command_t os_ignore_list = { "LIST", "List services ignores", PRIV_ADMIN, 0, os_cmd_ignore_list };
command_t os_ignore_clear = { "CLEAR", "Clear all services ignores", PRIV_ADMIN, 0, os_cmd_ignore_clear };

list_t *os_cmdtree;
list_t *os_helptree;
list_t os_ignore_cmds;
list_t svs_ignore_list;


void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_ignore, os_cmdtree);
	help_addentry(os_helptree, "IGNORE", "help/oservice/ignore", NULL);

	/* Sub-commands */
	command_add(&os_ignore_add, &os_ignore_cmds);
	command_add(&os_ignore_del, &os_ignore_cmds);
	command_add(&os_ignore_clear, &os_ignore_cmds);
	command_add(&os_ignore_list, &os_ignore_cmds);

	use_svsignore++;
}

void _moddeinit()
{
	command_delete(&os_ignore, os_cmdtree);
	help_delentry(os_helptree, "IGNORE");

	/* Sub-commands */
	command_delete(&os_ignore_add, &os_ignore_cmds);
	command_delete(&os_ignore_del, &os_ignore_cmds);
	command_delete(&os_ignore_list, &os_ignore_cmds);
	command_delete(&os_ignore_clear, &os_ignore_cmds);

	use_svsignore--;
}

static void os_cmd_ignore(sourceinfo_t *si, int parc, char *parv[])
{
	char *cmd = parv[0];
        command_t *c;

	if (!cmd)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");
		command_fail(si, fault_needmoreparams, "Syntax: IGNORE ADD|DEL|LIST|CLEAR <mask>");
		return;
	}

        c = command_find(&os_ignore_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, "Invalid command. Use \2/%s%s help\2 for a command listing.", (ircd->uses_rcommand == FALSE) ? "msg " : "", opersvs.me->disp);
		return;
	}

	command_exec(si->service, si, c, parc - 1, parv + 1);

}

static void os_cmd_ignore_add(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n;
        char *target = parv[0];
	char *reason = parv[1];
	svsignore_t *svsignore;

	if (target == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");
		command_fail(si, fault_needmoreparams, "Syntax: IGNORE ADD|DEL|LIST|CLEAR <mask>");
		return;
	}

	if (!validhostmask(target))
	{
		command_fail(si, fault_badparams, "Invalid host mask, %s", target);
		return;
	}

	/* Are we already ignoring this mask? */
	LIST_FOREACH(n, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		/* We're here */
		if (!strcasecmp(svsignore->mask, target))
		{
			command_fail(si, fault_nochange, "The mask \2%s\2 already exists on the services ignore list.", svsignore->mask);
			return;
		}
	}

	svsignore = svsignore_add(target, reason);
	svsignore->setby = sstrdup(get_oper_name(si));
	svsignore->settime = CURRTIME;

	command_success_nodata(si, "\2%s\2 has been added to the services ignore list.", target);

	logcommand(si, CMDLOG_ADMIN, "IGNORE ADD %s %s", target, reason);
	wallops("%s added a services ignore for \2%s\2 (%s).", get_oper_name(si), target, reason);
	snoop("IGNORE:ADD: \2%s\2 by \2%s\2 (%s)", target, get_oper_name(si), reason);

	return;
}

static void os_cmd_ignore_del(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	node_t *n, *tn;
	svsignore_t *svsignore;

	if (target == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");
		command_fail(si, fault_needmoreparams, "Syntax: IGNORE ADD|DEL|LIST|CLEAR <mask>");
		return;
	}

	LIST_FOREACH_SAFE(n, tn, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		if (!strcasecmp(svsignore->mask,target))
		{
			command_success_nodata(si, "\2%s\2 has been removed from the services ignore list.", svsignore->mask);

			svsignore_delete(svsignore);

			wallops("%s removed \2%s\2 from the services ignore list.", get_oper_name(si), target);
			snoop("IGNORE:DEL: \2%s\2 by \2%s\2", target, get_oper_name(si));
			logcommand(si, CMDLOG_ADMIN, "IGNORE DEL %s", target);

			return;
		}
	}

	command_fail(si, fault_nosuch_target, "\2%s\2 was not found on the services ignore list.", target);
	return;
}

static void os_cmd_ignore_clear(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n, *tn;
	svsignore_t *svsignore;

	if (LIST_LENGTH(&svs_ignore_list) == 0)
	{
		command_success_nodata(si, "Services ignore list is empty.");
		return;
	}

	LIST_FOREACH_SAFE(n, tn, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		command_success_nodata(si, "\2%s\2 has been removed from the services ignore list.", svsignore->mask);
		node_del(n,&svs_ignore_list);
		node_free(n);
		free(svsignore->mask);
		free(svsignore->setby);
		free(svsignore->reason);

	}

	command_success_nodata(si, "Services ignore list has been wiped!");

	wallops("\2%s\2 wiped the services ignore list.", get_oper_name(si));
	snoop("IGNORE:CLEAR: by \2%s\2", get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "IGNORE CLEAR");

	return;
}


static void os_cmd_ignore_list(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n;
	uint8_t i = 1;
	svsignore_t *svsignore;
	char strfbuf[32];
	struct tm tm;

	if (LIST_LENGTH(&svs_ignore_list) == 0)
	{
		command_success_nodata(si, "The services ignore list is empty.");
		return;
	}

	command_success_nodata(si, "Current Ignore list entries:");
	command_success_nodata(si, "-------------------------");

	LIST_FOREACH(n, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		tm = *localtime(&svsignore->settime);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		command_success_nodata(si, "%d: %s by %s on %s (Reason: %s)", i, svsignore->mask, svsignore->setby, strfbuf, svsignore->reason);
		i++;
	}

	command_success_nodata(si, "-------------------------");

	logcommand(si, CMDLOG_ADMIN, "IGNORE LIST");

	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
