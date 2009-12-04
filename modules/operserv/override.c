/*
 * Copyright (c) 2009 William Pitcock <nenolod@atheme.org>.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService INJECT command.
 *
 * $Id: inject.c 7877 2007-03-06 01:43:05Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/override", false, _modinit, _moddeinit,
	"$Id: inject.c 7877 2007-03-06 01:43:05Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_override(sourceinfo_t *si, int parc, char *parv[]);

command_t os_override = { "OVERRIDE", N_("Perform a transaction on another user's account"), PRIV_OVERRIDE, 4, os_cmd_override };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_override, os_cmdtree);
	help_addentry(os_helptree, "OVERRIDE", "help/oservice/override", NULL);
}

void _moddeinit(void)
{
	command_delete(&os_override, os_cmdtree);
	help_delentry(os_helptree, "OVERRIDE");
}

static int text_to_parv(char *text, int maxparc, char **parv)
{
	int count = 0;
	char *p;

	if (maxparc == 0)
		return 0;

	if (!text)
		return 0;

	p = text;
	while (count < maxparc - 1 && (parv[count] = strtok(p, " ")) != NULL)
		count++, p = NULL;

	if ((parv[count] = strtok(p, "")) != NULL)
	{
		p = parv[count];

		while (*p == ' ')
			p++;
		parv[count] = p;

		if (*p != '\0')
		{
			p += strlen(p) - 1;

			while (*p == ' ' && p > parv[count])
				p--;

			p[1] = '\0';
			count++;
		}
	}

	return count;
}

static void os_cmd_override(sourceinfo_t *si, int parc, char *parv[])
{
	sourceinfo_t o_si;
	myuser_t *mu;
	service_t *svs;
	command_t *cmd;
	int newparc, i;
	char *newparv[20];

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OVERRIDE");
		command_fail(si, fault_needmoreparams, _("Syntax: OVERRIDE <account> <service> <command> <params>"));
		return;
	}

	if (!(mu = myuser_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered account."), parv[0]);
		return;
	}

	svs = service_find_nick(parv[1]);
	if (svs == NULL || svs->cmdtree == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a valid service."), parv[1]);
		return;
	}

	cmd = command_find(svs->cmdtree, parv[2]);
	if (cmd == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a valid command."), parv[2]);
		return;
	}

	o_si.su = si->su;
	o_si.smu = mu;
	o_si.service = svs;

	newparc = text_to_parv(parv[3], cmd->maxparc, newparv);
	for (i = 0; i < (int)(sizeof(newparv) / sizeof(newparv[0])); i++)
		newparv[i] = NULL;
	command_exec(svs, &o_si, cmd, newparc, newparv);

	logcommand(si, CMDLOG_ADMIN, "OVERRIDE %s %s %s [%s]", parv[0], parv[1], parv[2], parv[3]);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
