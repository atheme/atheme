/* groupserv - group services.
 * Copyright (c) 2010 Atheme Development Group
 */

#include "groupserv.h"

/* This should probably be moved to privs.h or at least groupserv.h at some point */
#define PRIV_GROUP_ADMIN "group:admin"

/* I don't like this here, but it works --jdhore */
static void create_challenge(sourceinfo_t *si, const char *name, int v, char *dest)
{
	char buf[256];
	int digest[4];
	md5_state_t ctx;

	snprintf(buf, sizeof buf, "%lu:%s:%s",
			(unsigned long)(CURRTIME / 300) - v,
			get_source_name(si),
			name);
	md5_init(&ctx);
	md5_append(&ctx, (unsigned char *)buf, strlen(buf));
	md5_finish(&ctx, (unsigned char *)digest);
	/* note: this depends on byte order, but that's ok because
	 * it's only going to work in the same atheme instance anyway
	 */
	snprintf(dest, 80, "%x:%x", digest[0], digest[1]);
}

static void gs_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_help = { "HELP", N_("Displays contextual help information."), AC_NONE, 1, gs_cmd_help };

void gs_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), si->service->nick);
		command_success_nodata(si, _("\2%s\2 provides tools for managing groups of users and channels."), si->service->nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information on a command, type:"));
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		command_success_nodata(si, " ");

		command_help(si, &gs_cmdtree);

		command_success_nodata(si, _("***** \2End of Help\2 *****"));
		return;
	}

	/* take the command through the hash table */
	help_display(si, si->service, parv[0], &gs_helptree);
}

static void gs_cmd_register(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_register = { "REGISTER", N_("Registers a group."), AC_NONE, 2, gs_cmd_register };

static void gs_cmd_register(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGISTER");
		command_fail(si, fault_needmoreparams, _("To register a group: REGISTER <!groupname>"));
		return;
	}

	if (*parv[0] != '!')
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "REGISTER");
		command_fail(si, fault_needmoreparams, _("To register a group: REGISTER <!groupname>"));
		return;
	}

	if ((mg = mygroup_find(parv[0])) != NULL)
	{
		command_fail(si, fault_alreadyexists, _("The group \2%s\2 already exists."), parv[0]);
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (myuser_count_group_flag(si->smu, GA_FOUNDER) > maxgroups)
	{
		command_fail(si, fault_toomany, _("You have too many groups registered."));
		return;
	}

	mg = mygroup_add(parv[0]);
	groupacs_add(mg, si->smu, GA_ALL | GA_FOUNDER);

	logcommand(si, CMDLOG_REGISTER, "REGISTER: \2%s\2", entity(mg)->name); 
	command_success_nodata(si, _("The group \2%s\2 has been registered to \2%s\2."), entity(mg)->name, entity(si->smu)->name);
}

static void gs_cmd_info(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_info = { "INFO", N_("Displays information about registered groups."), AC_NONE, 2, gs_cmd_info };

static void gs_cmd_info(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;
	struct tm tm;
	char buf[BUFSIZE], strfbuf[32];
	metadata_t *md;

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INFO");
		command_fail(si, fault_needmoreparams, _("Syntax: INFO <!groupname>"));
		return;
	}

	if (!(mg = mygroup_find(parv[0])))
	{
		command_fail(si, fault_alreadyexists, _("Group \2%s\2 does not exist."), parv[0]);
		return;
	}

	tm = *localtime(&mg->regtime);
	strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

	command_success_nodata(si, _("Information for \2%s\2:"), parv[0]);
	command_success_nodata(si, _("Registered  : %s (%s ago)"), strfbuf, time_ago(mg->regtime));
	command_success_nodata(si, _("Founder     : %s"), mygroup_founder_names(mg));

	if ((md = metadata_find(mg, "description")))
		command_success_nodata(si, _("Description : %s"), md->value);
	if ((md = metadata_find(mg, "channel")))
		command_success_nodata(si, _("Channel     : %s"), md->value);
	if ((md = metadata_find(mg, "url")))
		command_success_nodata(si, _("URL         : %s"), md->value);
	if ((md = metadata_find(mg, "email")))
		command_success_nodata(si, _("Email       : %s"), md->value);

	*buf = '\0';

	if (mg->flags & MG_REGNOLIMIT)
		strcat(buf, "REGNOLIMIT");

	if (mg->flags & MG_ACSNOLIMIT)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "ACSNOLIMIT");
	}

	if (*buf)
		command_success_nodata(si, _("Flags       : %s"), buf);

	command_success_nodata(si, _("\2*** End of Info ***\2"));

	logcommand(si, CMDLOG_GET, "INFO: \2%s\2", parv[0]);
}

static void gs_cmd_list(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_list = { "LIST", N_("List all registered groups."), PRIV_GROUP_ADMIN, 1, gs_cmd_list };

static void gs_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	myentity_t *mt;
	myentity_iteration_state_t state;

	command_success_nodata(si, _("Groups currently registered:"));

	MYENTITY_FOREACH_T(mt, &state, ENT_GROUP)
	{
		mygroup_t *mg = group(mt);
		continue_if_fail(mt != NULL);
		continue_if_fail(mg != NULL);

		command_success_nodata(si, _("- %s (%s)"), entity(mg)->name, mygroup_founder_names(mg));
	}

	command_success_nodata(si, _("\2*** End of List ***\2"));
	logcommand(si, CMDLOG_GET, "LIST");
}

static void gs_cmd_drop(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_drop = { "DROP", N_("Drops a group registration."), AC_NONE, 2, gs_cmd_drop };

static void gs_cmd_drop(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;
	char *name = parv[0];
	char *key = parv[1];
	char fullcmd[512];
	char key0[80], key1[80];

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		command_fail(si, fault_needmoreparams, _("Syntax: DROP <!group>"));
		return;
	}

	if (*name != '!')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DROP");
		command_fail(si, fault_badparams, _("Syntax: DROP <!group>"));
		return;
	}

	if (!(mg = mygroup_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), name);
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (si->su != NULL)
	{
		if (!key)
		{
			create_challenge(si, entity(mg)->name, 0, key0);
			snprintf(fullcmd, sizeof fullcmd, "/%s%s DROP %s %s",
					(ircd->uses_rcommand == false) ? "msg " : "",
					si->service->disp, entity(mg)->name, key0);
			command_success_nodata(si, _("To avoid accidental use of this command, this operation has to be confirmed. Please confirm by replying with \2%s\2"),
					fullcmd);
			return;
		}
		/* accept current and previous key */
		create_challenge(si, entity(mg)->name, 0, key0);
		create_challenge(si, entity(mg)->name, 1, key1);
		if (strcmp(key, key0) && strcmp(key, key1))
		{
			command_fail(si, fault_badparams, _("Invalid key for %s."), "DROP");
			return;
		}
	}

	logcommand(si, CMDLOG_REGISTER, "DROP: \2%s\2", entity(mg)->name);
	object_unref(mg);
	command_success_nodata(si, _("The group \2%s\2 has been dropped."), name);
	return;
}

static void gs_cmd_flags(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_flags = { "FLAGS", N_("Sets flags on a user in a group."), AC_NONE, 3, gs_cmd_flags };

static void gs_cmd_flags(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;
	myuser_t *mu;
	groupacs_t *ga;
	unsigned int flags = 0;
	unsigned int dir;
	char *c;

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FLAGS");
		command_fail(si, fault_needmoreparams, _("Syntax: FLAGS <!group> [user] [changes]"));
		return;
	}

	if ((mg = mygroup_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), parv[0]);
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_FLAGS))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if (!parv[1])
	{
		node_t *n;
		int i = 1;

		command_success_nodata(si, _("Entry Account                Flags"));
		command_success_nodata(si, "----- ---------------------- -----");

		LIST_FOREACH(n, mg->acs.head)
		{
			ga = n->data;

			command_success_nodata(si, "%-5d %-22s %s", i, entity(ga->mu)->name,
					       gflags_tostr(ga_flags, ga->flags));

			i++;
		}

		command_success_nodata(si, "----- ---------------------- -----");
		command_success_nodata(si, _("End of \2%s\2 FLAGS listing."), parv[0]);
		logcommand(si, CMDLOG_GET, "FLAGS: \2%s\2", parv[0]);
		return;
	}

	if ((mu = myuser_find_ext(parv[1])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered account."), parv[1]);
		return;
	}

	if (!parv[2])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FLAGS");
		command_fail(si, fault_needmoreparams, _("Syntax: FLAGS <!group> <user> <changes>"));
		return;
	}

	ga = groupacs_find(mg, mu, 0);
	if (ga != NULL)
		flags = ga->flags;

	/* XXX: this sucks. :< */
	c = parv[2];
	while (*c)
	{
		switch(*c)
		{
		case '+':
			dir = 0;
			break;
		case '-':
			dir = 1;
			break;
		case '*':
			if (dir)
				flags = 0;
			else
				flags = GA_ALL;
			break;
		case 'F':
			if (dir)
				flags &= ~GA_FOUNDER;
			else
				flags |= GA_FOUNDER;
			break;
		case 'f':
			if (dir)
				flags &= ~GA_FLAGS;
			else
				flags |= GA_FLAGS;
			break;
		case 's':
			if (dir)
				flags &= ~GA_SET;
			else
				flags |= GA_SET;
			break;
		case 'v':
			if (dir)
				flags &= ~GA_VHOST;
			else
				flags |= GA_VHOST;
			break;
		case 'c':
			if (dir)
				flags &= ~GA_CHANACS;
			else
			{
				if (mu->flags & MU_NEVEROP)
				{
					command_fail(si, fault_noprivs, _("\2%s\2 does not wish to be added to channel access lists (NEVEROP set)."), entity(mu)->name);
					return;
				}
				flags |= GA_CHANACS;
			}
			break;
		case 'm':
			if (dir)
				flags &= ~GA_MEMOS;
			else
				flags |= GA_MEMOS;
			break;
		default:
			break;
		}

		c++;
	}

	if (ga != NULL && flags != 0)
		ga->flags = flags;
	else if (ga != NULL)
	{
		groupacs_delete(mg, mu);
		command_success_nodata(si, _("\2%s\2 has been removed from \2%s\2."), entity(mu)->name, entity(mg)->name);
		logcommand(si, CMDLOG_SET, "FLAGS:REMOVE: \2%s\2 on \2%s\2", entity(mu)->name, entity(mg)->name);
		return;
	}
	else 
	{
		if (LIST_LENGTH(&mg->acs) > maxgroupacs && (!(mg->flags & MG_ACSNOLIMIT)))
		{
			command_fail(si, fault_toomany, _("Group %s access list is full."), entity(mg)->name);
			return;
		}
		ga = groupacs_add(mg, mu, flags);
	}

	command_success_nodata(si, _("\2%s\2 now has flags \2%s\2 on \2%s\2."), entity(mu)->name, gflags_tostr(ga_flags, ga->flags), entity(mg)->name);

	/* XXX */
	logcommand(si, CMDLOG_SET, "FLAGS: \2%s\2 now has flags \2%s\2 on \2%s\2", entity(mu)->name, gflags_tostr(ga_flags,  ga->flags), entity(mg)->name);
}

static void gs_cmd_regnolimit(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_regnolimit = { "REGNOLIMIT", N_("Allow a group to bypass registration limits."), PRIV_GROUP_ADMIN, 2, gs_cmd_regnolimit };

static void gs_cmd_regnolimit(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;

	if (!parv[0] && !parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGNOLIMIT");
		command_fail(si, fault_needmoreparams, _("Syntax: REGNOLIMIT <!group> <ON|OFF>"));
		return;
	}

	if ((mg = mygroup_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), parv[0]);
		return;
	}
	
	if (!strcasecmp(parv[1], "ON"))
	{
		if (mg->flags & MG_REGNOLIMIT)
		{
			command_fail(si, fault_nochange, _("\2%s\2 can already bypass registration limits."), entity(mg)->name);
			return;
		}

		mg->flags |= MG_REGNOLIMIT;

		wallops("%s set the REGNOLIMIT option on the group \2%s\2.", get_oper_name(si), entity(mg)->name);
		logcommand(si, CMDLOG_ADMIN, "REGNOLIMIT:ON: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 can now bypass registration limits."), entity(mg)->name);
	}
	else if (!strcasecmp(parv[1], "OFF"))
	{
		if (!(mg->flags & MG_REGNOLIMIT))
		{
			command_fail(si, fault_nochange, _("\2%s\2 cannot bypass registration limits."), entity(mg)->name);
			return;
		}

		mg->flags &= ~MG_REGNOLIMIT;

		wallops("%s removed the REGNOLIMIT option from the group \2%s\2.", get_oper_name(si), entity(mg)->name);
		logcommand(si, CMDLOG_ADMIN, "REGNOLIMIT:OFF: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 cannot bypass registration limits anymore."), entity(mg)->name);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "REGNOLIMIT");
		command_fail(si, fault_badparams, _("Syntax: REGNOLIMIT <!group> <ON|OFF>"));
	}
}

static void gs_cmd_acsnolimit(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_acsnolimit = { "ACSNOLIMIT", N_("Allow a group to bypass access list limits."), PRIV_GROUP_ADMIN, 2, gs_cmd_acsnolimit };

static void gs_cmd_acsnolimit(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;

	if (!parv[0] && !parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACSNOLIMIT");
		command_fail(si, fault_needmoreparams, _("Syntax: ACSNOLIMIT <!group> <ON|OFF>"));
		return;
	}

	if ((mg = mygroup_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), parv[0]);
		return;
	}
	
	if (!strcasecmp(parv[1], "ON"))
	{
		if (mg->flags & MG_ACSNOLIMIT)
		{
			command_fail(si, fault_nochange, _("\2%s\2 can already bypass access list limits."), entity(mg)->name);
			return;
		}

		mg->flags |= MG_ACSNOLIMIT;

		wallops("%s set the ACSNOLIMIT option on the group \2%s\2.", get_oper_name(si), entity(mg)->name);
		logcommand(si, CMDLOG_ADMIN, "ACSNOLIMIT:ON: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 can now bypass access list limits."), entity(mg)->name);
	}
	else if (!strcasecmp(parv[1], "OFF"))
	{
		if (!(mg->flags & MG_ACSNOLIMIT))
		{
			command_fail(si, fault_nochange, _("\2%s\2 cannot bypass access list limits."), entity(mg)->name);
			return;
		}

		mg->flags &= ~MG_ACSNOLIMIT;

		wallops("%s removed the ACSNOLIMIT option from the group \2%s\2.", get_oper_name(si), entity(mg)->name);
		logcommand(si, CMDLOG_ADMIN, "ACSNOLIMIT:OFF: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 cannot bypass access list limits anymore."), entity(mg)->name);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "ACSNOLIMIT");
		command_fail(si, fault_badparams, _("Syntax: ACSNOLIMIT <!group> <ON|OFF>"));
	}
}

void basecmds_init(void)
{
	command_add(&gs_help, &gs_cmdtree);
	command_add(&gs_register, &gs_cmdtree);
	command_add(&gs_info, &gs_cmdtree);
	command_add(&gs_list, &gs_cmdtree);
	command_add(&gs_drop, &gs_cmdtree);
	command_add(&gs_flags, &gs_cmdtree);
	command_add(&gs_regnolimit, &gs_cmdtree);
	command_add(&gs_acsnolimit, &gs_cmdtree);

	help_addentry(&gs_helptree, "HELP", "help/help", NULL);
	help_addentry(&gs_helptree, "REGISTER", "help/groupserv/register", NULL);
	help_addentry(&gs_helptree, "INFO", "help/groupserv/info", NULL);
	help_addentry(&gs_helptree, "LIST", "help/groupserv/list", NULL);
	help_addentry(&gs_helptree, "DROP", "help/groupserv/drop", NULL);
	help_addentry(&gs_helptree, "FLAGS", "help/groupserv/flags", NULL);
	help_addentry(&gs_helptree, "REGNOLIMIT", "help/groupserv/regnolimit", NULL);
	help_addentry(&gs_helptree, "ACSNOLIMIT", "help/groupserv/acsnolimit", NULL);
}

void basecmds_deinit(void)
{
	command_delete(&gs_help, &gs_cmdtree);
	command_delete(&gs_register, &gs_cmdtree);
	command_delete(&gs_info, &gs_cmdtree);
	command_delete(&gs_list, &gs_cmdtree);
	command_delete(&gs_drop, &gs_cmdtree);
	command_delete(&gs_flags, &gs_cmdtree);
	command_delete(&gs_regnolimit, &gs_cmdtree);
	command_delete(&gs_acsnolimit, &gs_cmdtree);

	help_delentry(&gs_helptree, "HELP");
	help_delentry(&gs_helptree, "REGISTER");
	help_delentry(&gs_helptree, "INFO");
	help_delentry(&gs_helptree, "LIST");
	help_delentry(&gs_helptree, "DROP");
	help_delentry(&gs_helptree, "FLAGS");
	help_delentry(&gs_helptree, "REGNOLIMIT");
	help_delentry(&gs_helptree, "ACSNOLIMIT");
}

