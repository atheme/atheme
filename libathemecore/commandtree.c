/*
 * atheme-services: A collection of minimalist IRC services   
 * commandtree.c: Management of services commands.
 *
 * Copyright (c) 2005-2010 Atheme Project (http://www.atheme.org)           
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"
#include "privs.h"

static int text_to_parv(char *text, int maxparc, char **parv);

void command_add(command_t *cmd, mowgli_patricia_t *commandtree)
{
	return_if_fail(cmd != NULL);
	return_if_fail(commandtree != NULL);

	mowgli_patricia_add(commandtree, cmd->name, cmd);
}

void command_delete(command_t *cmd, mowgli_patricia_t *commandtree)
{
	return_if_fail(cmd != NULL);
	return_if_fail(commandtree != NULL);

	mowgli_patricia_delete(commandtree, cmd->name);
}

command_t *command_find(mowgli_patricia_t *commandtree, const char *command)
{
	return_val_if_fail(commandtree != NULL, NULL);
	return_val_if_fail(command != NULL, NULL);

	return mowgli_patricia_retrieve(commandtree, command);
}

void command_exec(service_t *svs, sourceinfo_t *si, command_t *c, int parc, char *parv[])
{
	if (si->smu != NULL)
		language_set_active(si->smu->language);

	if (has_priv(si, c->access))
	{
		if (si->force_language != NULL)
			language_set_active(si->force_language);

		c->cmd(si, parc, parv);
		language_set_active(NULL);
		return;
	}

	if (has_any_privs(si))
		command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, c->access);
	else
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
	/* logcommand(si, CMDLOG_ADMIN, "DENIED COMMAND: \2%s\2 used \2%s\2 \2%s\2", origin, svs->name, cmd); */
	if (si->smu != NULL)
		language_set_active(NULL);
}

void command_exec_split(service_t *svs, sourceinfo_t *si, const char *cmd, char *text, mowgli_patricia_t *commandtree)
{
	int parc, i;
	char *parv[20];
        command_t *c;

	cmd = service_resolve_alias(svs, commandtree == svs->commands ? NULL : "unknown", cmd);
	if ((c = command_find(commandtree, cmd)))
	{
		parc = text_to_parv(text, c->maxparc, parv);
		for (i = parc; i < (int)(sizeof(parv) / sizeof(parv[0])); i++)
			parv[i] = NULL;
		command_exec(svs, si, c, parc, parv);
	}
	else
	{
		if (si->smu != NULL)
			language_set_active(si->smu->language);

		notice(svs->nick, si->su->nick, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", svs->disp);

		if (si->smu != NULL)
			language_set_active(NULL);
	}
}

/*
 * command_help
 *     Iterates the command tree and lists available commands.
 *
 * inputs -
 *     si:          The origin of the request.
 *     commandtree: The command tree being listed.
 * 
 * outputs -
 *     A list of available commands.
 */
void command_help(sourceinfo_t *si, mowgli_patricia_t *commandtree)
{
	mowgli_patricia_iteration_state_t state;
	command_t *c;

	if (si->service == NULL || si->service->commands == commandtree)
		command_success_nodata(si, _("The following commands are available:"));
	else
		command_success_nodata(si, _("The following subcommands are available:"));

	MOWGLI_PATRICIA_FOREACH(c, &state, commandtree)
	{
		/* show only the commands we have access to
		 * (taken from command_exec())
		 */
		if (has_priv(si, c->access))
			command_success_nodata(si, "\2%-15s\2 %s", c->name, translation_get(_(c->desc)));
	}
}

/* name1 name2 name3... */
static bool string_in_list(const char *str, const char *name)
{
	char *p;
	int l;

	if (str == NULL)
		return false;
	l = strlen(name);
	while (*str != '\0')
	{
		p = strchr(str, ' ');
		if (p != NULL ? p - str == l && !strncasecmp(str, name, p - str) : !strcasecmp(str, name))
			return true;
		if (p == NULL)
			return false;
		str = p;
		while (*str == ' ')
			str++;
	}
	return false;
}

/*
 * command_help_short
 *     Iterates over the command tree and lists available commands.
 *
 * inputs -
 *     mynick:      The nick of the services bot sending out the notices.
 *     origin:      The origin of the request.
 *     commandtree: The command tree being listed.
 *     maincmds:    The commands to list verbosely.
 * 
 * outputs -
 *     A list of available commands.
 */
void command_help_short(sourceinfo_t *si, mowgli_patricia_t *commandtree, const char *maincmds)
{
	mowgli_patricia_iteration_state_t state;
	unsigned int l, lv;
	char buf[256], *p;
	command_t *c;

	if (si->service == NULL || si->service->commands == commandtree)
		command_success_nodata(si, _("The following commands are available:"));
	else
		command_success_nodata(si, _("The following subcommands are available:"));

	MOWGLI_PATRICIA_FOREACH(c, &state, commandtree)
	{
		/* show only the commands we have access to
		 * (taken from command_exec())
		 */
		if (string_in_list(maincmds, c->name) && has_priv(si, c->access))
			command_success_nodata(si, "\2%-15s\2 %s", c->name, translation_get(_(c->desc)));
	}

	command_success_nodata(si, " ");
	strlcpy(buf, translation_get(_("\2Other commands:\2 ")), sizeof buf);
	l = strlen(buf);
	lv = 0;
	for (p = buf; *p != '\0'; p++)
	{
		if (!(*p >= '\1' && *p < ' '))
			lv++;
	}

	MOWGLI_PATRICIA_FOREACH(c, &state, commandtree)
	{
		/* show only the commands we have access to
		 * (taken from command_exec())
		 */
		if (!string_in_list(maincmds, c->name) && has_priv(si, c->access))
		{
			if (strlen(buf) > l)
				strlcat(buf, ", ", sizeof buf);
			if (strlen(buf) > 55)
			{
				command_success_nodata(si, "%s", buf);
				l = lv;
				buf[lv] = '\0';
				while (--lv > 0)
					buf[lv] = ' ';
				buf[0] = ' ';
				lv = l;
			}
			strlcat(buf, c->name, sizeof buf);
		}
	}
	if (strlen(buf) > l)
		command_success_nodata(si, "%s", buf);
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
