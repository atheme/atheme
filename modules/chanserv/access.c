/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * ACCESS command implementation for ChanServ.
 */

#include "atheme.h"
#include "template.h"

DECLARE_MODULE_V1
(
	"chanserv/access", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_access(sourceinfo_t *si, int parc, char *parv[]);
static void cs_help_access(sourceinfo_t *si, char *subcmd);

command_t cs_access = { "ACCESS", N_("Manage channel access."),
                        AC_NONE, 20, cs_cmd_access, { .func = cs_help_access } };

mowgli_patricia_t *cs_access_cmds;

void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_access);

	cs_access_cmds = mowgli_patricia_create(strcasecanon);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_access);

	mowgli_patricia_destroy(cs_access_cmds, NULL, NULL);
}

static const char *get_template_name_fuzzy(mychan_t *mc, unsigned int level)
{
	metadata_t *md;
	const char *p, *q, *r;
	char *s;
	char ss[40];
	static char flagname[400];

	md = metadata_find(mc, "private:templates");
	if (md != NULL)
	{
		p = md->value;
		while (p != NULL)
		{
			while (*p == ' ')
				p++;
			q = strchr(p, '=');
			if (q == NULL)
				break;
			r = strchr(q, ' ');
			if (r != NULL && r < q)
				break;
			strlcpy(ss, q, sizeof ss);
			if (r != NULL && r - q < (int)(sizeof ss - 1))
			{
				ss[r - q] = '\0';
			}
			if (level & flags_to_bitmask(ss, 0))
			{
				strlcpy(flagname, p, sizeof flagname);
				s = strchr(flagname, '=');
				if (s != NULL)
					*s = '\0';
				return flagname;
			}
			p = r;
		}
	}

	if (level & chansvs.ca_sop || level & get_template_flags(mc, "SOP"))
		return "SOP";
	if (level & chansvs.ca_aop || level & get_template_flags(mc, "AOP"))
		return "AOP";
	/* if vop==hop, prefer vop */
	if (level & chansvs.ca_vop || level & get_template_flags(mc, "VOP"))
		return "VOP";
	if (chansvs.ca_hop != chansvs.ca_vop && (level & chansvs.ca_hop ||
			level & get_template_flags(mc, "HOP")))
		return "HOP";
	return NULL;
}

static const char *get_template_name(mychan_t *mc, unsigned int level)
{
	metadata_t *md;
	const char *p, *q, *r;
	char *s;
	char ss[40];
	static char flagname[400];

	md = metadata_find(mc, "private:templates");
	if (md != NULL)
	{
		p = md->value;
		while (p != NULL)
		{
			while (*p == ' ')
				p++;
			q = strchr(p, '=');
			if (q == NULL)
				break;
			r = strchr(q, ' ');
			if (r != NULL && r < q)
				break;
			strlcpy(ss, q, sizeof ss);
			if (r != NULL && r - q < (int)(sizeof ss - 1))
			{
				ss[r - q] = '\0';
			}
			if (level == flags_to_bitmask(ss, 0))
			{
				strlcpy(flagname, p, sizeof flagname);
				s = strchr(flagname, '=');
				if (s != NULL)
					*s = '\0';
				return flagname;
			}
			p = r;
		}
	}
	if (level == chansvs.ca_sop && level == get_template_flags(mc, "SOP"))
		return "SOP";
	if (level == chansvs.ca_aop && level == get_template_flags(mc, "AOP"))
		return "AOP";
	/* if vop==hop, prefer vop */
	if (level == chansvs.ca_vop && level == get_template_flags(mc, "VOP"))
		return "VOP";
	if (chansvs.ca_hop != chansvs.ca_vop && level == chansvs.ca_hop &&
			level == get_template_flags(mc, "HOP"))
		return "HOP";
	return NULL;
}

static void cs_help_access(sourceinfo_t *si, char *subcmd)
{
	if (!subcmd)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), chansvs.me->disp);
		command_success_nodata(si, _("Help for \2ACCESS\2:"));
		command_success_nodata(si, " ");
		command_help(si, cs_access_cmds);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information, use \2/msg %s HELP ACCESS \37command\37\2."), chansvs.me->disp);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
	}
	else
		help_display(si, si->service, subcmd, cs_access_cmds);
}

static void cs_cmd_access(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan;
	char *cmd;
	command_t *c;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS");
		command_fail(si, fault_needmoreparams, _("Syntax: ACCESS <#channel> <command> [parameters]"));
		return;
	}
	
	if (parv[0][0] == '#')
		chan = parv[0], cmd = parv[1];
	else if (parv[1][0] == '#')
		cmd = parv[0], chan = parv[1];
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "ACCESS");
		command_fail(si, fault_badparams, _("Syntax: ACCESS <#channel> <command> [parameters]"));
		return;
	}

	c = command_find(cs_access_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", chansvs.me->disp);
		return;
	}

	parv[1] = chan;
	command_exec(si->service, si, c, parc - 1, parv + 1);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
