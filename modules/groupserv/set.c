/* groupserv.c - group services
 * Copyright (C) 2010 Atheme Development Group
 */

#include "atheme.h"
#include "groupserv.h"

static void gs_help_set(sourceinfo_t *si, const char *subcmd);
static void gs_cmd_set(sourceinfo_t *si, int parc, char *parv[]);
static void gs_cmd_set_email(sourceinfo_t *si, int parc, char *parv[]);
static void gs_cmd_set_url(sourceinfo_t *si, int parc, char *parv[]);
static void gs_cmd_set_description(sourceinfo_t *si, int parc, char *parv[]);
static void gs_cmd_set_channel(sourceinfo_t *si, int parc, char *parv[]);
static void gs_cmd_set_open(sourceinfo_t *si, int parc, char *parv[]);
static void gs_cmd_set_public(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_set = { "SET", N_("Sets various control flags."), AC_NONE, 3, gs_cmd_set, { .func = gs_help_set } };
command_t gs_set_email = { "EMAIL", N_("Sets the group e-mail address."), AC_NONE, 2, gs_cmd_set_email, { .path = "groupserv/set_email" } };
command_t gs_set_url = { "URL", N_("Sets the group URL."), AC_NONE, 2, gs_cmd_set_url, { .path = "groupserv/set_url" } };
command_t gs_set_description = { "DESCRIPTION", N_("Sets the group description."), AC_NONE, 2, gs_cmd_set_description, { .path = "groupserv/set_description" } };
command_t gs_set_channel = { "CHANNEL", N_("Sets the official group channel."), AC_NONE, 2, gs_cmd_set_channel, { .path = "groupserv/set_channel" } };
command_t gs_set_open = { "OPEN", N_("Sets the group as open for anyone to join."), AC_NONE, 2, gs_cmd_set_open, { .path = "groupserv/set_open" } };
command_t gs_set_public = { "PUBLIC", N_("Sets the group as public."), AC_NONE, 2, gs_cmd_set_public, { .path = "groupserv/set_public" } };

mowgli_patricia_t *gs_set_cmdtree;

void set_init(void)
{
	service_bind_command(groupsvs, &gs_set);

	gs_set_cmdtree = mowgli_patricia_create(strcasecanon);

	command_add(&gs_set_email, gs_set_cmdtree);
	command_add(&gs_set_url, gs_set_cmdtree);
	command_add(&gs_set_description, gs_set_cmdtree);
	command_add(&gs_set_channel, gs_set_cmdtree);
	command_add(&gs_set_open, gs_set_cmdtree);
	command_add(&gs_set_public, gs_set_cmdtree);
}

void set_deinit(void)
{
	service_unbind_command(groupsvs, &gs_set);

	command_delete(&gs_set_email, gs_set_cmdtree);
	command_delete(&gs_set_url, gs_set_cmdtree);
	command_delete(&gs_set_description, gs_set_cmdtree);
	command_delete(&gs_set_channel, gs_set_cmdtree);
	command_delete(&gs_set_open, gs_set_cmdtree);
	command_delete(&gs_set_public, gs_set_cmdtree);

	mowgli_patricia_destroy(gs_set_cmdtree, NULL, NULL);
}

static void gs_help_set(sourceinfo_t *si, const char *subcmd)
{
	if (!subcmd)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), si->service->disp);
		command_success_nodata(si, _("Help for \2SET\2:"));
		command_success_nodata(si, " ");
		command_success_nodata(si, _("SET allows you to set various control flags\n"
					"for groups that change the way certain\n"
					"operations are performed on them."));
		command_success_nodata(si, " ");
		command_help(si, gs_set_cmdtree);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more specific help use \2/msg %s HELP SET \37command\37\2."), si->service->disp);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
	}
	else
		help_display(si, si->service, subcmd, gs_set_cmdtree);
}

/* SET <!group> <setting> <parameters> */
static void gs_cmd_set(sourceinfo_t *si, int parc, char *parv[])
{
	char *group;
	char *cmd;
	command_t *c;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <!group> <setting> [parameters]"));
		return;
	}

	if (parv[0][0] == '!')
		group = parv[0], cmd = parv[1];
	else if (parv[1][0] == '!')
		cmd = parv[0], group = parv[1];
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET");
		command_fail(si, fault_badparams, _("Syntax: SET <!group> <setting> [parameters]"));
		return;
	}

	c = command_find(gs_set_cmdtree, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		return;
	}

	parv[1] = group;
	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void gs_cmd_set_email(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;
	char *mail = parv[1];

	if (!(mg = mygroup_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), parv[0]);
		return;
	}
	
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (!mail || !strcasecmp(mail, "NONE") || !strcasecmp(mail, "OFF"))
	{
		if (metadata_find(mg, "email"))
		{
			metadata_delete(mg, "email");
			command_success_nodata(si, _("The e-mail address for group \2%s\2 was deleted."), entity(mg)->name);
			logcommand(si, CMDLOG_SET, "SET:EMAIL:NONE: \2%s\2", entity(mg)->name);
			return;
		}

		command_fail(si, fault_nochange, _("The e-mail address for group \2%s\2 was not set."), entity(mg)->name);
		return;
	}

	if (!validemail(mail))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid e-mail address."), mail);
		return;
	}

	/* we'll overwrite any existing metadata */
	metadata_add(mg, "email", mail);

	logcommand(si, CMDLOG_SET, "SET:EMAIL: \2%s\2 \2%s\2", entity(mg)->name, mail);
	command_success_nodata(si, _("The e-mail address for group \2%s\2 has been set to \2%s\2."), parv[0], mail);
}

static void gs_cmd_set_url(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;
	char *url = parv[1];

	if (!(mg = mygroup_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), parv[0]);
		return;
	}
	
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (!url || !strcasecmp("OFF", url) || !strcasecmp("NONE", url))
	{
		/* not in a namespace to allow more natural use of SET PROPERTY.
		 * they may be able to introduce spaces, though. c'est la vie.
		 */
		if (metadata_find(mg, "url"))
		{
			metadata_delete(mg, "url");
			logcommand(si, CMDLOG_SET, "SET:URL:NONE: \2%s\2", entity(mg)->name);
			command_success_nodata(si, _("The URL for \2%s\2 has been cleared."), parv[0]);
			return;
		}

		command_fail(si, fault_nochange, _("The URL for \2%s\2 was not set."), parv[0]);
		return;
	}

	/* we'll overwrite any existing metadata */
	metadata_add(mg, "url", url);

	logcommand(si, CMDLOG_SET, "SET:URL: \2%s\2 \2%s\2", entity(mg)->name, url);
	command_success_nodata(si, _("The URL of \2%s\2 has been set to \2%s\2."), parv[0], url);
}

static void gs_cmd_set_description(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;
	char *desc = parv[1];

	if (!(mg = mygroup_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), parv[0]);
		return;
	}
	
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (!desc || !strcasecmp("OFF", desc) || !strcasecmp("NONE", desc))
	{
		/* not in a namespace to allow more natural use of SET PROPERTY.
		 * they may be able to introduce spaces, though. c'est la vie.
		 */
		if (metadata_find(mg, "description"))
		{
			metadata_delete(mg, "description");
			logcommand(si, CMDLOG_SET, "SET:DESCRIPTION:NONE: \2%s\2", entity(mg)->name);
			command_success_nodata(si, _("The description for \2%s\2 has been cleared."), parv[0]);
			return;
		}

		command_fail(si, fault_nochange, _("A description for \2%s\2 was not set."), parv[0]);
		return;
	}

	/* we'll overwrite any existing metadata */
	metadata_add(mg, "description", desc);

	logcommand(si, CMDLOG_SET, "SET:DESCRIPTION: \2%s\2 \2%s\2", entity(mg)->name, desc);
	command_success_nodata(si, _("The description of \2%s\2 has been set to \2%s\2."), parv[0], desc);
}

static void gs_cmd_set_channel(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;
	char *chan = parv[1];

	if (!(mg = mygroup_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), parv[0]);
		return;
	}
	
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (!chan || !strcasecmp("OFF", chan) || !strcasecmp("NONE", chan))
	{
		/* not in a namespace to allow more natural use of SET PROPERTY.
		 * they may be able to introduce spaces, though. c'est la vie.
		 */
		if (metadata_find(mg, "channel"))
		{
			metadata_delete(mg, "channel");
			logcommand(si, CMDLOG_SET, "SET:CHANNEL:NONE: \2%s\2", entity(mg)->name);
			command_success_nodata(si, _("The official channel for \2%s\2 has been cleared."), parv[0]);
			return;
		}

		command_fail(si, fault_nochange, _("A official channel for \2%s\2 was not set."), parv[0]);
		return;
	}

	/* we'll overwrite any existing metadata */
	metadata_add(mg, "channel", chan);

	logcommand(si, CMDLOG_SET, "SET:CHANNEL: \2%s\2 \2%s\2", entity(mg)->name, chan);
	command_success_nodata(si, _("The official channel of \2%s\2 has been set to \2%s\2."), parv[0], chan);
}

static void gs_cmd_set_open(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;

	if (!parv[0] || !parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OPEN");
		command_fail(si, fault_needmoreparams, _("Syntax: OPEN <!group> <ON|OFF>"));
		return;
	}

	if ((mg = mygroup_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), parv[0]);
		return;
	}
	
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}
	
	if (!groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (!strcasecmp(parv[1], "ON"))
	{
		if (!enable_open_groups)
		{
			command_fail(si, fault_nochange, _("Setting groups as open has been administratively disabled."));
			return;
		}

		if (mg->flags & MG_OPEN)
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already open to anyone joining."), entity(mg)->name);
			return;
		}

		mg->flags |= MG_OPEN;

		logcommand(si, CMDLOG_SET, "OPEN:ON: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 is now open to anyone joining."), entity(mg)->name);
	}
	else if (!strcasecmp(parv[1], "OFF"))
	{
		if (!(mg->flags & MG_OPEN))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already not open to anyone joining."), entity(mg)->name);
			return;
		}

		mg->flags &= ~MG_OPEN;

		logcommand(si, CMDLOG_SET, "OPEN:OFF: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 is no longer open to anyone joining."), entity(mg)->name);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "OPEN");
		command_fail(si, fault_badparams, _("Syntax: OPEN <!group> <ON|OFF>"));
	}
}

static void gs_cmd_set_public(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;

	if (!parv[0] || !parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "PUBLIC");
		command_fail(si, fault_needmoreparams, _("Syntax: PUBLIC <!group> <ON|OFF>"));
		return;
	}

	if ((mg = mygroup_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), parv[0]);
		return;
	}
	
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}
	
	if (!groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (!strcasecmp(parv[1], "ON"))
	{
		if (mg->flags & MG_PUBLIC)
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already public."), entity(mg)->name);
			return;
		}

		mg->flags |= MG_PUBLIC;

		logcommand(si, CMDLOG_SET, "PUBLIC:ON: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 is now public."), entity(mg)->name);
	}
	else if (!strcasecmp(parv[1], "OFF"))
	{
		if (!(mg->flags & MG_PUBLIC))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is not public already."), entity(mg)->name);
			return;
		}

		mg->flags &= ~MG_PUBLIC;

		logcommand(si, CMDLOG_SET, "PUBLIC:OFF: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 is no longer public."), entity(mg)->name);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "PUBLIC");
		command_fail(si, fault_badparams, _("Syntax: PUBLIC <!group> <ON|OFF>"));
	}
}
/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
