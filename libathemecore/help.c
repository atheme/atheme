/*
 * atheme-services: A collection of minimalist IRC services   
 * help.c: Help system implementation.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)           
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

static bool command_has_help(command_t *cmd)
{
	return_val_if_fail(cmd != NULL, false);

	return (cmd->help.func != NULL || cmd->help.path != NULL);
}

static command_t *help_cmd_find(sourceinfo_t *si, const char *cmd, mowgli_patricia_t *list)
{
	command_t *c;

	if ((c = mowgli_patricia_retrieve(list, cmd)) != NULL && command_has_help(c))
		return c;

	command_fail(si, fault_nosuch_target, _("No help available for \2%s\2."), cmd);

	/* Fun for helpchan/helpurl. */
	if (config_options.helpchan && config_options.helpurl)
		command_fail(si, fault_nosuch_target, _("If you're having trouble, you may want to join the help channel %s or visit the help webpage %s"), config_options.helpchan, config_options.helpurl);
	else if (config_options.helpchan && !config_options.helpurl)
		command_fail(si, fault_nosuch_target, _("If you're having trouble, you may want to join the help channel %s"), config_options.helpchan);
	else if (!config_options.helpchan && config_options.helpurl)
		command_fail(si, fault_nosuch_target, _("If you're having trouble, you may want to visit the help webpage %s"), config_options.helpurl);

	return NULL;
}

static bool evaluate_condition(sourceinfo_t *si, const char *s)
{
	char word[80];
	char *p, *q;

	while (*s == ' ' || *s == '\t')
		s++;
	if (*s == '!')
		return !evaluate_condition(si, s + 1);
	strlcpy(word, s, sizeof word);
	p = strchr(word, ' ');
	if (p != NULL)
	{
		*p++ = '\0';
		while (*p == ' ' || *p == '\t')
			p++;
	}
	if (!strcmp(word, "halfops"))
		return ircd->uses_halfops;
	else if (!strcmp(word, "owner"))
		return ircd->uses_owner;
	else if (!strcmp(word, "protect"))
		return ircd->uses_protect;
	else if (!strcmp(word, "anyprivs"))
		return has_any_privs(si);
	else if (!strcmp(word, "priv"))
	{
		q = strchr(p, ' ');
		if (q != NULL)
			*q = '\0';
		return has_priv(si, p);
	}
	else if (!strcmp(word, "module"))
	{
		q = strchr(p, ' ');
		if (q != NULL)
			*q = '\0';
		return module_find_published(p) != NULL;
	}
	else if (!strcmp(word, "auth"))
		return me.auth != AUTH_NONE;
	else
		return false;
}

void help_display(sourceinfo_t *si, service_t *service, const char *command, mowgli_patricia_t *list)
{
	command_t *c;
	FILE *help_file = NULL;
	char subname[BUFSIZE], buf[BUFSIZE];
	const char *langname = NULL;
	int ifnest, ifnest_false;


	char *ccommand = sstrdup(command);
	char *subcmd = strtok(ccommand, " ");
	subcmd = strtok(NULL, "");

	/* take the command through the hash table */
	if ((c = help_cmd_find(si, ccommand, list)))
	{
		if (c->help.path)
		{
			if (*c->help.path == '/')
				help_file = fopen(c->help.path, "r");
			else
			{
				strlcpy(subname, c->help.path, sizeof subname);
				if (nicksvs.no_nick_ownership && !strncmp(subname, "nickserv/", 9))
					memcpy(subname, "userserv", 8);
				if (si->smu != NULL)
				{
					langname = language_get_real_name(si->smu->language);
					if (!strcmp(langname, "en"))
						langname = NULL;
				}
				if (langname != NULL)
				{
					snprintf(buf, sizeof buf, "%s/%s/%s", SHAREDIR "/help", langname, subname);
					help_file = fopen(buf, "r");
				}
				if (help_file == NULL)
				{
					snprintf(buf, sizeof buf, "%s/%s", SHAREDIR "/help", subname);
					help_file = fopen(buf, "r");
				}
			}

			if (!help_file)
			{
				command_fail(si, fault_nosuch_target, _("Could not get help file for \2%s\2."), command);
				return;
			}

			command_success_nodata(si, _("***** \2%s Help\2 *****"), service->nick);

			ifnest = ifnest_false = 0;
			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", service->disp);
				if (!strncmp(buf, "#if", 3))
				{
					if (ifnest_false > 0 || !evaluate_condition(si, buf + 3))
						ifnest_false++;
					ifnest++;
					continue;
				}
				else if (!strncmp(buf, "#endif", 6))
				{
					if (ifnest_false > 0)
						ifnest_false--;
					if (ifnest > 0)
						ifnest--;
					continue;
				}
				else if (!strncmp(buf, "#else", 5))
				{
					if (ifnest > 0 && ifnest_false <= 1)
						ifnest_false ^= 1;
					continue;
				}
				if (ifnest_false > 0)
					continue;

				if (buf[0])
					command_success_nodata(si, "%s", buf);
				else
					command_success_nodata(si, " ");
			}

			fclose(help_file);

			command_success_nodata(si, _("***** \2End of Help\2 *****"));
		}
		else if (c->help.func)
		{
			/* I removed the "***** Help" stuff from here because everything
			 * that uses a help function now calls help_display so they'll
			 * display on that and not show the message twice. --JD
			 */
			c->help.func(si, subcmd);
		}
		else
			command_fail(si, fault_nosuch_target, _("No help available for \2%s\2."), command);
	}
	free(ccommand);
}

void help_addentry(mowgli_list_t *list, const char *topic, const char *fname,
	void (*func)(sourceinfo_t *si))
{
	slog(LG_DEBUG, "help_addentry(): unimplemented stub, port to new command framework");
}

void help_delentry(mowgli_list_t *list, const char *name)
{
	slog(LG_DEBUG, "help_delentry(): unimplemented stub, port to new command framework");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
