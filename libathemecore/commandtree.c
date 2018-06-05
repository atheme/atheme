/*
 * atheme-services: A collection of minimalist IRC services
 * commandtree.c: Management of services commands.
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
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

static bool permissive_mode_fallback = false;

static int
text_to_parv(char *text, int maxparc, char **parv)
{
	int count = 0;
	char *p;

	if (maxparc == 0)
		return 0;

	if (!text)
		return 0;

	p = text;
	while (count < maxparc - 1 && (parv[count] = strtok(p, " ")) != NULL)
	{
		count++;
		p = NULL;
	}

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

static bool
default_cmd_auth(struct service *svs, struct sourceinfo *si, struct command *c, const char *userlevel)
{
	if (! (has_priv(si, c->access) && has_priv(si, userlevel)))
	{
		if (!permissive_mode_fallback)
			logaudit_denycmd(si, c, userlevel);
		return false;
	}

	return true;
}

static inline bool
command_verify(struct service *svs, struct sourceinfo *si, struct command *c, const char *userlevel)
{
	if (command_authorize(svs, si, c, userlevel))
		return true;

	if (permissive_mode)
	{
		permissive_mode_fallback = true;

		const bool ret = default_cmd_auth(svs, si, c, userlevel);

		permissive_mode_fallback = false;

		return ret;
	}

	return false;
}

void
command_add(struct command *cmd, mowgli_patricia_t *commandtree)
{
	return_if_fail(cmd != NULL);
	return_if_fail(commandtree != NULL);

	mowgli_patricia_add(commandtree, cmd->name, cmd);
}

void
command_delete(struct command *cmd, mowgli_patricia_t *commandtree)
{
	return_if_fail(cmd != NULL);
	return_if_fail(commandtree != NULL);

	mowgli_patricia_delete(commandtree, cmd->name);
}

void
command_delete_trie_cb(const char ATHEME_VATTR_UNUSED *const restrict cmdname, void *const restrict cmd,
                       void *const restrict cmdlist)
{
	(void) command_delete(cmd, cmdlist);
}

struct command *
command_find(mowgli_patricia_t *commandtree, const char *command)
{
	return_val_if_fail(commandtree != NULL, NULL);
	return_val_if_fail(command != NULL, NULL);

	return mowgli_patricia_retrieve(commandtree, command);
}

void
command_exec(struct service *svs, struct sourceinfo *si, struct command *c, int parc, char *parv[])
{
	const char *cmdaccess;

	if (si->smu != NULL)
		language_set_active(si->smu->language);

	/* Make this look a bit more expected for normal users */
	if (si->smu == NULL && c->access != NULL && !strcasecmp(c->access, AC_AUTHENTICATED))
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		language_set_active(NULL);
		return;
	}

	cmdaccess = service_set_access(svs, c->name, c->access);

	if (command_verify(svs, si, c, cmdaccess))
	{
		if (si->force_language != NULL)
			language_set_active(si->force_language);

		si->command = c;
		c->cmd(si, parc, parv);
		language_set_active(NULL);
		return;
	}

	if (has_any_privs(si))
	{
		char accessbuf[BUFSIZE];

		*accessbuf = '\0';

		if (c->access && (!cmdaccess || strcmp(c->access, cmdaccess)))
		{
			mowgli_strlcat(accessbuf, c->access, BUFSIZE);
			mowgli_strlcat(accessbuf, " ", BUFSIZE);
		}

		mowgli_strlcat(accessbuf, cmdaccess, BUFSIZE);

		command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, accessbuf);
	}
	else
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
	/* logcommand(si, CMDLOG_ADMIN, "DENIED COMMAND: \2%s\2 used \2%s\2 \2%s\2", origin, svs->name, cmd); */
	if (si->smu != NULL)
		language_set_active(NULL);
}

void
command_exec_split(struct service *svs, struct sourceinfo *si, const char *cmd, char *text, mowgli_patricia_t *commandtree)
{
	int parc, i;
	char *parv[20];
        struct command *c;

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

		(void) help_display_invalid(si, svs, NULL);

		if (si->smu != NULL)
			language_set_active(NULL);
	}
}

void
subcommand_dispatch_simple(struct service *const restrict svs, struct sourceinfo *const restrict si, const int parc,
                           char **const restrict parv, mowgli_patricia_t *const restrict commandtree,
                           const char *const restrict subcmd)
{
	struct command *const cmd = command_find(commandtree, parv[0]);

	if (cmd)
		(void) command_exec(svs, si, cmd, parc - 1, parv + 1);
	else
		(void) help_display_invalid(si, svs, subcmd);
}

bool (*command_authorize)(struct service *, struct sourceinfo *, struct command *, const char *) = &default_cmd_auth;
