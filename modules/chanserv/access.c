/*
 * Copyright (c) 2010 - 2011 William Pitcock <nenolod@atheme.org>.
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
static void cs_help_access(sourceinfo_t *si, const char *subcmd);

command_t cs_access = { "ACCESS", N_("Manage channel access."),
                        AC_NONE, 3, cs_cmd_access, { .func = cs_help_access } };

static void cs_cmd_role(sourceinfo_t *si, int parc, char *parv[]);
static void cs_help_role(sourceinfo_t *si, const char *subcmd);

command_t cs_role =  { "ROLE", N_("Manage channel roles."),
                        AC_NONE, 3, cs_cmd_role, { .func = cs_help_role } };

static void cs_cmd_access_list(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_access_list = { "LIST", N_("List channel access entries."),
                             AC_NONE, 1, cs_cmd_access_list, { .path = "cservice/access_list" } };

static void cs_cmd_access_info(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_access_info = { "INFO", N_("Display information on an access list entry."),
                             AC_NONE, 2, cs_cmd_access_info, { .path = "cservice/access_info" } };

static void cs_cmd_access_del(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_access_del =  { "DEL", N_("Delete an access list entry."),
                             AC_NONE, 2, cs_cmd_access_del, { .path = "cservice/access_del" } };

static void cs_cmd_access_add(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_access_add =  { "ADD", N_("Add an access list entry."),
                             AC_NONE, 3, cs_cmd_access_add, { .path = "cservice/access_add" } };

static void cs_cmd_access_set(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_access_set =  { "SET", N_("Update an access list entry."),
                             AC_NONE, 3, cs_cmd_access_set, { .path = "cservice/access_set" } };

static void cs_cmd_role_list(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_role_list = { "LIST", N_("List available roles."),
                            AC_NONE, 1, cs_cmd_role_list, { .path = "cservice/role_list" } };

static void cs_cmd_role_add(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_role_add =  { "ADD", N_("Add a role."),
                            AC_NONE, 20, cs_cmd_role_add, { .path = "cservice/role_add" } };

static void cs_cmd_role_set(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_role_set =  { "SET", N_("Change flags on a role."),
                            AC_NONE, 20, cs_cmd_role_set, { .path = "cservice/role_set" } };

static void cs_cmd_role_del(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_role_del =  { "DEL", N_("Delete a role."),
                            AC_NONE, 2, cs_cmd_role_del, { .path = "cservice/role_del" } };

mowgli_patricia_t *cs_access_cmds;
mowgli_patricia_t *cs_role_cmds;

void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_access);
	service_named_bind_command("chanserv", &cs_role);

	cs_access_cmds = mowgli_patricia_create(strcasecanon);
	cs_role_cmds  = mowgli_patricia_create(strcasecanon);

	command_add(&cs_access_list, cs_access_cmds);
	command_add(&cs_access_info, cs_access_cmds);
	command_add(&cs_access_del, cs_access_cmds);
	command_add(&cs_access_add, cs_access_cmds);
	command_add(&cs_access_set, cs_access_cmds);

	command_add(&cs_role_list, cs_role_cmds);
	command_add(&cs_role_add, cs_role_cmds);
	command_add(&cs_role_set, cs_role_cmds);
	command_add(&cs_role_del, cs_role_cmds);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_access);
	service_named_unbind_command("chanserv", &cs_role);

	mowgli_patricia_destroy(cs_access_cmds, NULL, NULL);
	mowgli_patricia_destroy(cs_role_cmds, NULL, NULL);
}

static void cs_help_access(sourceinfo_t *si, const char *subcmd)
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

static void cs_help_role(sourceinfo_t *si, const char *subcmd)
{
	if (!subcmd)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), chansvs.me->disp);
		command_success_nodata(si, _("Help for \2ROLE\2:"));
		command_success_nodata(si, " ");
		command_help(si, cs_role_cmds);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information, use \2/msg %s HELP ROLE \37command\37\2."), chansvs.me->disp);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
	}
	else
		help_display(si, si->service, subcmd, cs_role_cmds);
}

static void cs_cmd_access(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan;
	char *cmd;
	command_t *c;
	char buf[BUFSIZE];

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

	if (parc > 2)
		snprintf(buf, BUFSIZE, "%s %s", chan, parv[2]);
	else
		mowgli_strlcpy(buf, chan, BUFSIZE);

	command_exec_split(si->service, si, c->name, buf, cs_access_cmds);
}

static void cs_cmd_role(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan;
	char *cmd;
	command_t *c;
	char buf[BUFSIZE];

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ROLE");
		command_fail(si, fault_needmoreparams, _("Syntax: ROLE <#channel> <command> [parameters]"));
		return;
	}

	if (parv[0][0] == '#')
		chan = parv[0], cmd = parv[1];
	else if (parv[1][0] == '#')
		cmd = parv[0], chan = parv[1];
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "ROLE");
		command_fail(si, fault_badparams, _("Syntax: ROLE <#channel> <command> [parameters]"));
		return;
	}

	c = command_find(cs_role_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", chansvs.me->disp);
		return;
	}

	if (parc > 2)
		snprintf(buf, BUFSIZE, "%s %s", chan, parv[2]);
	else
		mowgli_strlcpy(buf, chan, BUFSIZE);

	command_exec_split(si->service, si, c->name, buf, cs_role_cmds);
}

/***********************************************************************************************/

static inline unsigned int count_bits(unsigned int bits)
{
	unsigned int i = 1, count = 0;

	for (i = 1; i < sizeof(bits) * 8; i++)
	{
		if (bits & (1 << i))
			count++;
	}

	return count;
}

typedef struct {
	char name[400];
	unsigned int level;
	mowgli_node_t node;
} template_t;

typedef struct {
	mychan_t *mc;
	mowgli_list_t *l;
} template_iter_t;

static int compare_template_nodes(mowgli_node_t *a, mowgli_node_t *b, void *opaque)
{
	template_t *ta = a->data;
	template_t *tb = b->data;

	return count_bits(ta->level) - count_bits(tb->level);
}

static template_t *find_template(mowgli_list_t *l, const char *key)
{
	mowgli_node_t *iter;

	return_val_if_fail(l != NULL, NULL);
	return_val_if_fail(key != NULL, NULL);

	MOWGLI_ITER_FOREACH(iter, l->head)
	{
		template_t *t = iter->data;

		if (!strcasecmp(t->name, key))
			return t;
	}

	return NULL;
}

static int append_global_template(const char *key, void *data, void *privdata)
{
	template_t *t;
	template_iter_t *ti = privdata;
	default_template_t *def_t = data;
	unsigned int vopflags;

	if (!chansvs.hide_xop)
	{
		vopflags = get_global_template_flags("VOP");

		if (def_t->flags == vopflags && !strcasecmp(key, "HOP"))
			return 0;
	}

	if ((t = find_template(ti->l, key)) != NULL)
		return 0;

	t = smalloc(sizeof(template_t));
	mowgli_strlcpy(t->name, key, sizeof(t->name));
	t->level = get_template_flags(ti->mc, key);
	mowgli_node_add(t, &t->node, ti->l);

        return 0;
}

static mowgli_list_t *build_template_list(mychan_t *mc)
{
	const char *p, *q, *r;
	char *s;
	char ss[40];
	static char flagname[400];
	metadata_t *md;
	mowgli_list_t *l;
	template_t *t;
	template_iter_t ti;

	l = mowgli_list_create();

	return_val_if_fail(mc != NULL, l);

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
			mowgli_strlcpy(ss, q, sizeof ss);
			if (r != NULL && r - q < (int)(sizeof ss - 1))
			{
				ss[r - q] = '\0';
			}

			mowgli_strlcpy(flagname, p, sizeof flagname);
			s = strchr(flagname, '=');
			if (s != NULL)
				*s = '\0';

			t = smalloc(sizeof(template_t));
			mowgli_strlcpy(t->name, flagname, sizeof(t->name));
			t->level = flags_to_bitmask(ss, 0);
			mowgli_node_add(t, &t->node, l);

			p = r;
		}
	}

	ti.l = l;
	ti.mc = mc;
	mowgli_patricia_foreach(global_template_dict, append_global_template, &ti);

	mowgli_list_sort(l, compare_template_nodes, NULL);

	/* reverse the list so that we go from highest flags to lowest. */
	mowgli_list_reverse(l);

	return l;
}

static void free_template_list(mowgli_list_t *l)
{
	mowgli_node_t *n, *tn;

	return_if_fail(l != NULL);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, l->head)
	{
		template_t *t = n->data;

		mowgli_node_delete(&t->node, l);
		free(t);
	}
}

/*
 * get_template_name()
 *
 * this was, by far, one of the most complicated functions in Atheme.
 * it was also one of the most bloated fuzzy bitmask matchers that i have
 * ever written.
 *
 * basically, we used to build a template list, which has been sorted and then
 * reversed.  we then find three templates that are the closest match to:
 *
 *      - an exact match (level == level)
 *      - the closest template to the level but with less privilege ((level & t->level) == level)
 *      - the closest template to the level but with more privilege ((t->level & level) == t->level)
 *
 * we then counted the number of bits in each level and we tightened the match further
 * based on number of bits flipped on in each level.  (it was the only sane way to do
 * this, trust me.)
 *
 * but now we just return an exact template name or <Custom>.
 */
static const char *get_template_name(mychan_t *mc, unsigned int level)
{
	mowgli_list_t *l;
	mowgli_node_t *n;
	static char flagname[400];
	template_t *exact_t = NULL;

	l = build_template_list(mc);

	/* find exact_t, lesser_t and greater_t */
	MOWGLI_ITER_FOREACH(n, l->head)
	{
		template_t *t = n->data;

		if (t->level == level)
			exact_t = t;
	}

	if (exact_t != NULL)
		mowgli_strlcpy(flagname, exact_t->name, sizeof flagname);
	else
		mowgli_strlcpy(flagname, "<Custom>", sizeof flagname);

	free_template_list(l);

	return flagname;
}

/*
 * Update a role entry and synchronize the changes with the access list.
 */
static void update_role_entry(sourceinfo_t *si, mychan_t *mc, const char *role, unsigned int flags)
{
	metadata_t *md;
	size_t l;
	char *p, *q, *r;
	char ss[40], newstr[400];
	bool found = false;
	unsigned int oldflags;
	char *flagstr;
	mowgli_node_t *n, *tn;
	chanacs_t *ca;
	int changes = 0;
	hook_channel_acl_req_t req;

	flagstr = bitmask_to_flags2(flags, 0);
	oldflags = get_template_flags(mc, role);
	l = strlen(role);

	md = metadata_find(mc, "private:templates");
	if (md != NULL)
	{
		p = md->value;
		mowgli_strlcpy(newstr, p, sizeof newstr);
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
			mowgli_strlcpy(ss, q, sizeof ss);
			if (r != NULL && r - q < (int)(sizeof ss - 1))
			{
				ss[r - q] = '\0';
			}
			if ((size_t)(q - p) == l && !strncasecmp(role, p, l))
			{
				found = true;

				if (flags == 0)
				{
					if (p == md->value)
						mowgli_strlcpy(newstr, r != NULL ? r + 1 : "", sizeof newstr);
					else
					{
						/* otherwise, zap the space before it */
						p--;
						mowgli_strlcpy(newstr + (p - md->value), r != NULL ? r : "", sizeof newstr - (p - md->value));
					}
				}
				else
					snprintf(newstr + (p - md->value), sizeof newstr - (p - md->value), "%s=%s%s", role, flagstr, r != NULL ? r : "");
				break;
			}
			p = r;
		}
	}

	if (!found)
	{
		if (md != NULL)
			snprintf(newstr + strlen(newstr), sizeof newstr - strlen(newstr), " %s=%s", role, flagstr);
		else
			snprintf(newstr, sizeof newstr, "%s=%s", role, flagstr);
	}

	if (oldflags == 0 && has_ctrl_chars(role))
	{
		command_fail(si, fault_badparams, _("Invalid template name \2%s\2."), role);
		return;
	}

	if (strlen(newstr) >= 300)
	{
		command_fail(si, fault_toomany, _("Sorry, too many templates on \2%s\2."), mc->name);
		return;
	}

	if (newstr[0] == '\0')
		metadata_delete(mc, "private:templates");
	else
		metadata_add(mc, "private:templates", newstr);

	if (flags)
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chanacs.head)
		{
			ca = n->data;
			if (ca->level != oldflags)
				continue;

			/* don't change foundership status. */
			if ((oldflags ^ flags) & CA_FOUNDER)
				continue;

			req.ca = ca;
			req.oldlevel = ca->level;

			changes++;
			chanacs_modify_simple(ca, flags, ~flags);

			req.newlevel = ca->level;

			hook_call_channel_acl_change(&req);
			chanacs_close(ca);
		}
	}

	logcommand(si, CMDLOG_SET, "ROLE:MOD: \2%s\2 \2%s\2 !\2%s\2 (\2%d\2 changes)", mc->name, role, flagstr, changes);

	if (changes > 0)
		command_success_nodata(si, _("%d access entries updated accordingly."), changes);
}

static unsigned int xflag_apply_batch(unsigned int in, int parc, char *parv[])
{
	unsigned int out;
	int i;

	out = in;
	for (i = 0; i < parc; i++)
		out = xflag_apply(out, parv[i]);

	return out;
}

/***********************************************************************************************/

/*
 * Syntax: ACCESS #channel LIST
 *
 * Output:
 *
 * Entry Nickname/Host          Role
 * ----- ---------------------- ----
 * 1     nenolod                founder
 * 2     jdhore                 channel-staffer
 */
static void cs_cmd_access_list(sourceinfo_t *si, int parc, char *parv[])
{
	chanacs_t *ca;
	mowgli_node_t *n;
	mychan_t *mc;
	const char *channel = parv[0];
	bool operoverride = false;
	unsigned int i = 1;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
	{
		if (has_priv(si, PRIV_CHAN_AUSPEX))
			operoverride = true;
		else
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}
	}

	command_success_nodata(si, _("Entry Nickname/Host          Role"));
	command_success_nodata(si, "----- ---------------------- ----");

	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		const char *role;

		ca = n->data;

		if (ca->level == CA_AKICK)
			continue;

		role = get_template_name(mc, ca->level);

		command_success_nodata(si, _("%-5d %-22s %s"), i, ca->entity ? ca->entity->name : ca->host, role);

		i++;
	}

	command_success_nodata(si, "----- ---------------------- ----");
	command_success_nodata(si, _("End of \2%s\2 ACCESS listing."), channel);

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "ACCESS:LIST: \2%s\2 (oper override)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "ACCESS:LIST: \2%s\2", mc->name);
}

/*
 * Syntax: ACCESS #channel INFO user
 *
 * Output:
 *
 * Access for nenolod in #atheme:
 * Role       : Founder
 * Flags      : aclview, ...
 * Modified   : Aug 18 21:41:31 2005 (5 years, 6 weeks, 0 days, 05:57:21 ago)
 * *** End of Info ***
 */
static void cs_cmd_access_info(sourceinfo_t *si, int parc, char *parv[])
{
	chanacs_t *ca;
	myentity_t *mt;
	mychan_t *mc;
	const char *channel = parv[0];
	const char *target = parv[1];
	bool operoverride = false;
	const char *role;
	struct tm tm;
	char strfbuf[BUFSIZE];
	metadata_t *md;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS INFO");
		command_fail(si, fault_needmoreparams, _("Syntax: ACCESS <#channel> INFO <account|hostmask>"));
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
	{
		if (has_priv(si, PRIV_CHAN_AUSPEX))
			operoverride = true;
		else
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}
	}

	if (validhostmask(target))
		ca = chanacs_find_host_literal(mc, target, 0);
	else
	{
		if (!(mt = myentity_find_ext(target)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
			return;
		}
		target = mt->name;
		ca = chanacs_find_literal(mc, mt, 0);
	}

	if (ca == NULL)
	{
		command_success_nodata(si, _("No ACL entry for \2%s\2 in \2%s\2 was found."), target, channel);

		if (operoverride)
			logcommand(si, CMDLOG_ADMIN, "ACCESS:INFO: \2%s\2 on \2%s\2 (oper override)", mc->name, target);
		else
			logcommand(si, CMDLOG_GET, "ACCESS:INFO: \2%s\2 on \2%s\2", mc->name, target);

		return;
	}

	role = get_template_name(mc, ca->level);

	tm = *localtime(&ca->tmodified);
	strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, &tm);

	command_success_nodata(si, _("Access for \2%s\2 in \2%s\2:"), target, channel);

	if (ca->level == CA_AKICK)
	{
		md = metadata_find(ca, "reason");
		if (md != NULL)
			command_success_nodata(si, _("Ban reason : %s"),
					md->value);
	}
	else if (ca->entity && strcasecmp(target, ca->entity->name))
		command_success_nodata(si, _("Role       : %s (inherited from \2%s\2)"), role, ca->entity->name);
	else
		command_success_nodata(si, _("Role       : %s"), role);

	command_success_nodata(si, _("Flags      : %s (%s)"), xflag_tostr(ca->level), bitmask_to_flags2(ca->level, 0));
	command_success_nodata(si, _("Modified   : %s (%s ago)"), strfbuf, time_ago(ca->tmodified));
	command_success_nodata(si, _("*** \2End of Info\2 ***"));

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "ACCESS:INFO: \2%s\2 on \2%s\2 (oper override)", mc->name, target);
	else
		logcommand(si, CMDLOG_GET, "ACCESS:INFO: \2%s\2 on \2%s\2", mc->name, target);
}

/*
 * Syntax: ACCESS #channel DEL user
 *
 * Output:
 *
 * nenolod was removed from the Founder role in #atheme.
 */
static void cs_cmd_access_del(sourceinfo_t *si, int parc, char *parv[])
{
	chanacs_t *ca;
	myentity_t *mt;
	mychan_t *mc;
	hook_channel_acl_req_t req;
	unsigned int restrictflags;
	const char *channel = parv[0];
	const char *target = parv[1];
	const char *role;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS DEL");
		command_fail(si, fault_needmoreparams, _("Syntax: ACCESS <#channel> DEL <account|hostmask>"));
		return;
	}

	mt = myentity_find_ext(target);

	/* allow a user to resign their access even without FLAGS access. this is
	 * consistent with the other commands. --nenolod
	 */
	if (!chanacs_source_has_flag(mc, si, CA_FLAGS) && entity(si->smu) != mt)
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if (validhostmask(target))
		ca = chanacs_open(mc, NULL, target, true, entity(si->smu));
	else
	{
		if (mt == NULL)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
			return;
		}
		target = mt->name;
		ca = chanacs_open(mc, mt, NULL, true, entity(si->smu));
	}

	if (ca->level == 0)
	{
		chanacs_close(ca);

		command_success_nodata(si, _("No ACL entry for \2%s\2 in \2%s\2 was found."), target, channel);
		return;
	}

	if (ca->level & CA_FOUNDER && mychan_num_founders(mc) == 1)
	{
		command_fail(si, fault_noprivs, _("You may not remove the last founder."));
		return;
	}

	role = get_template_name(mc, ca->level);

	req.ca = ca;
	req.oldlevel = ca->level;
	req.newlevel = 0;

	restrictflags = chanacs_source_flags(mc, si);
	if (restrictflags & CA_FOUNDER)
		restrictflags = ca_all;
	else
	{
		if (restrictflags & CA_AKICK && entity(si->smu) == mt)
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}

		if (entity(si->smu) != mt)
			restrictflags = allow_flags(mc, restrictflags);
	}

	if (!chanacs_modify(ca, &req.newlevel, &req.oldlevel, restrictflags))
	{
		command_fail(si, fault_noprivs, _("You may not remove \2%s\2 from the \2%s\2 role."), target, role);
		return;
	}

	hook_call_channel_acl_change(&req);
	chanacs_close(ca);

	command_success_nodata(si, _("\2%s\2 was removed from the \2%s\2 role in \2%s\2."), target, role, channel);
	verbose(mc, "\2%s\2 deleted \2%s\2 from the access list (had role: \2%s\2).", get_source_name(si), target, role);

	logcommand(si, CMDLOG_SET, "ACCESS:DEL: \2%s\2 from \2%s\2", target, mc->name);
}

/*
 * Syntax: ACCESS #channel ADD user role
 *
 * Output:
 *
 * nenolod was added with the Founder role in #atheme.
 */
static void cs_cmd_access_add(sourceinfo_t *si, int parc, char *parv[])
{
	chanacs_t *ca;
	myentity_t *mt = NULL;
	mychan_t *mc;
	hook_channel_acl_req_t req;
	unsigned int oldflags, restrictflags;
	unsigned int newflags, addflags, removeflags;
	const char *channel = parv[0];
	const char *target = parv[1];
	const char *role = parv[2];

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!target || !role)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS ADD");
		command_fail(si, fault_needmoreparams, _("Syntax: ACCESS <#channel> ADD <account|hostmask> <role>"));
		return;
	}

	/* allow a user to resign their access even without FLAGS access. this is
	 * consistent with the other commands. --nenolod
	 */
	if (!chanacs_source_has_flag(mc, si, CA_FLAGS))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if (validhostmask(target))
		ca = chanacs_open(mc, NULL, target, true, entity(si->smu));
	else
	{
		if (!(mt = myentity_find_ext(target)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
			return;
		}
		target = mt->name;
		ca = chanacs_open(mc, mt, NULL, true, entity(si->smu));
	}

	if (ca->level == 0 && chanacs_is_table_full(ca))
	{
		chanacs_close(ca);
		command_fail(si, fault_toomany, _("Channel %s access list is full."), mc->name);
		return;
	}

	if (ca->level != 0)
	{
		chanacs_close(ca);
		command_fail(si, fault_toomany, _("\2%s\2 already has the \2%s\2 role in \2%s\2."), target, get_template_name(mc, ca->level), mc->name);
		return;
	}

	req.ca = ca;
	req.oldlevel = ca->level;

	newflags = get_template_flags(mc, role);
	if (newflags == 0)
	{
		chanacs_close(ca);
		command_fail(si, fault_toomany, _("Role \2%s\2 does not exist."), role);
		return;
	}

	restrictflags = chanacs_source_flags(mc, si);
	if (restrictflags & CA_FOUNDER)
		restrictflags = ca_all;
	else
	{
		if (restrictflags & CA_AKICK && entity(si->smu) == mt)
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}

		if (mt != entity(si->smu))
			restrictflags = allow_flags(mc, restrictflags);
		else
			restrictflags |= allow_flags(mc, restrictflags);
	}

	if (!(ca->level & CA_FOUNDER) && newflags & CA_FOUNDER)
	{
		if (mychan_num_founders(mc) >= chansvs.maxfounders)
		{
			command_fail(si, fault_noprivs, _("Only %d founders allowed per channel."), chansvs.maxfounders);
			chanacs_close(ca);
			return;
		}
		if (mt == NULL)
		{
			command_fail(si, fault_badparams, _("You may not set founder status on a hostmask."));
			chanacs_close(ca);
			return;
		}
		if (!myentity_can_register_channel(mt))
		{
			command_fail(si, fault_toomany, _("\2%s\2 has too many channels registered."), mt->name);
			chanacs_close(ca);
			return;
		}
	}

	oldflags = ca->level;

	addflags = newflags & ~oldflags;
	removeflags = ca_all & ~newflags;

	if (!chanacs_modify(ca, &addflags, &removeflags, restrictflags))
	{
		command_fail(si, fault_noprivs, _("You may not add \2%s\2 to the \2%s\2 role."), target, role);
		return;
	}

	req.newlevel = newflags;

	hook_call_channel_acl_change(&req);
	chanacs_close(ca);

	command_success_nodata(si, _("\2%s\2 was added with the \2%s\2 role in \2%s\2."), target, role, channel);
	verbose(mc, "\2%s\2 added \2%s\2 to the access list (with role: \2%s\2).", get_source_name(si), target, role);

	logcommand(si, CMDLOG_SET, "ACCESS:ADD: \2%s\2 to \2%s\2 as \2%s\2", target, mc->name, role);
}

/*
 * Syntax: ACCESS #channel SET user role
 *
 * Output:
 *
 * nenolod now has the Founder role in #atheme.
 */
static void cs_cmd_access_set(sourceinfo_t *si, int parc, char *parv[])
{
	chanacs_t *ca;
	myentity_t *mt = NULL;
	mychan_t *mc;
	hook_channel_acl_req_t req;
	unsigned int oldflags, restrictflags;
	unsigned int newflags, addflags, removeflags;
	const char *channel = parv[0];
	const char *target = parv[1];
	const char *role = parv[2];

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!target || !role)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS SET");
		command_fail(si, fault_needmoreparams, _("Syntax: ACCESS <#channel> SET <account|hostmask> <role>"));
		return;
	}

	/* allow a user to resign their access even without FLAGS access. this is
	 * consistent with the other commands. --nenolod
	 */
	if (!chanacs_source_has_flag(mc, si, CA_FLAGS))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if (validhostmask(target))
		ca = chanacs_open(mc, NULL, target, true, entity(si->smu));
	else
	{
		if (!(mt = myentity_find_ext(target)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
			return;
		}
		target = mt->name;
		ca = chanacs_open(mc, mt, NULL, true, entity(si->smu));
	}

	if (ca->level == 0 && chanacs_is_table_full(ca))
	{
		chanacs_close(ca);
		command_fail(si, fault_toomany, _("Channel \2%s\2 access list is full."), mc->name);
		return;
	}

	newflags = get_template_flags(mc, role);

	if (newflags == 0)
	{
		chanacs_close(ca);
		command_fail(si, fault_toomany, _("Role \2%s\2 does not exist."), role);
		return;
	}

	if ((ca->level & CA_FOUNDER) && !(newflags & CA_FOUNDER) && mychan_num_founders(mc) == 1)
	{
		command_fail(si, fault_noprivs, _("You may not remove the last founder."));
		return;
	}

	req.ca = ca;
	req.oldlevel = ca->level;

	restrictflags = chanacs_source_flags(mc, si);
	if (restrictflags & CA_FOUNDER)
		restrictflags = ca_all;
	else
	{
		if (restrictflags & CA_AKICK && entity(si->smu) == mt)
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}

		if (mt != entity(si->smu))
			restrictflags = allow_flags(mc, restrictflags);
		else
			restrictflags |= allow_flags(mc, restrictflags);
	}

	if (!(ca->level & CA_FOUNDER) && newflags & CA_FOUNDER)
	{
		if (mychan_num_founders(mc) >= chansvs.maxfounders)
		{
			command_fail(si, fault_noprivs, _("Only %d founders allowed per channel."), chansvs.maxfounders);
			chanacs_close(ca);
			return;
		}
		if (mt == NULL)
		{
			command_fail(si, fault_badparams, _("You may not set founder status on a hostmask."));
			chanacs_close(ca);
			return;
		}
		if (!myentity_can_register_channel(mt))
		{
			command_fail(si, fault_toomany, _("\2%s\2 has too many channels registered."), mt->name);
			chanacs_close(ca);
			return;
		}
	}

	oldflags = ca->level;

	addflags = newflags & ~oldflags;
	removeflags = ca_all & ~newflags;

	if (!chanacs_modify(ca, &addflags, &removeflags, restrictflags))
	{
		command_fail(si, fault_noprivs, _("You may not add \2%s\2 to the \2%s\2 role."), target, role);
		return;
	}

	req.newlevel = newflags;

	hook_call_channel_acl_change(&req);
	chanacs_close(ca);

	command_success_nodata(si, _("\2%s\2 now has the \2%s\2 role in \2%s\2."), target, role, channel);
	verbose(mc, "\2%s\2 changed the access list role for \2%s\2 to \2%s\2.", get_source_name(si), target, role);

	logcommand(si, CMDLOG_SET, "ACCESS:SET: \2%s\2 to \2%s\2 as \2%s\2", target, mc->name, role);
}

/*
 * Syntax: ROLE #channel LIST
 *
 * Output:
 *
 * List of network-wide roles:
 * SOP          : ...
 * AOP          : ...
 * HOP          : ...
 * VOP          : ...
 *
 * List of channel-defined roles:
 * Founder      : ...
 */
static void cs_cmd_role_list(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	const char *channel = parv[0];
	mowgli_list_t *l;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	l = build_template_list(mc);
	if (l != NULL)
	{
		mowgli_node_t *n;

		command_success_nodata(si, _("List of channel-defined roles:"));

		MOWGLI_ITER_FOREACH(n, l->head)
		{
			template_t *t = n->data;

			command_success_nodata(si, "%-20s: %s (%s)", t->name, xflag_tostr(t->level), bitmask_to_flags(t->level));
		}

		free_template_list(l);
	}
}

/*
 * Syntax: ROLE #channel ADD <role> [flags-changes]
 *
 * Output:
 *
 * Creates a new role with the given flags.
 */
static void cs_cmd_role_add(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	mowgli_list_t *l;
	const char *channel = parv[0];
	const char *role = parv[1];
	unsigned int oldflags, newflags, restrictflags;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!role)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ROLE ADD|SET");
		command_fail(si, fault_needmoreparams, _("Syntax: ROLE <#channel> ADD|SET <role> [flags]"));
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_FLAGS))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	restrictflags = chanacs_source_flags(mc, si);
	if (restrictflags & CA_FOUNDER)
		restrictflags = ca_all;
	else
		restrictflags = allow_flags(mc, restrictflags);
	oldflags = get_template_flags(mc, role);

	if (oldflags != 0)
	{
		command_fail(si, fault_badparams, _("Role \2%s\2 already exists."), role);
		return;
	}

	newflags = xflag_apply_batch(oldflags, parc - 2, parv + 2);
	if (newflags & ~restrictflags)
	{
		unsigned int delta = newflags & ~restrictflags;

		command_fail(si, fault_badparams, _("You do not have appropriate permissions to add flags: \2%s\2"), xflag_tostr(delta));
		return;
	}

	if (newflags & CA_FOUNDER)
		newflags |= CA_FLAGS;

	if (newflags == 0)
	{
		if (oldflags == 0)
			command_fail(si, fault_badparams, _("No valid flags given, use /%s%s HELP ROLE ADD for a list"), ircd->uses_rcommand ? "" : "msg ", chansvs.me->disp);
		else
			command_fail(si, fault_nosuch_target, _("You cannot remove all flags from the role \2%s\2."), role);
		return;
	}
	l = build_template_list(mc);
	if (l != NULL)
	{
		mowgli_node_t *n;

		MOWGLI_ITER_FOREACH(n, l->head)
		{
			template_t *t = n->data;

			if (t->level == newflags)
			{
				command_fail(si, fault_alreadyexists, _("The role \2%s\2 already has flags \2%s\2."), t->name, xflag_tostr(newflags));
				return;
			}
		}

		free_template_list(l);
	}

	command_success_nodata(si, _("Flags for role \2%s\2 were changed to: \2%s\2."), role, xflag_tostr(newflags));
	update_role_entry(si, mc, role, newflags);
}

/*
 * Syntax: ROLE #channel SET <role> [flags-changes]
 *
 * Output:
 *
 * Creates a new role with the given flags.
 */
static void cs_cmd_role_set(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	mowgli_list_t *l;
	const char *channel = parv[0];
	const char *role = parv[1];
	unsigned int oldflags, newflags, restrictflags;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!role)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ROLE ADD|SET");
		command_fail(si, fault_needmoreparams, _("Syntax: ROLE <#channel> ADD|SET <role> [flags]"));
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_FLAGS))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	restrictflags = chanacs_source_flags(mc, si);
	if (restrictflags & CA_FOUNDER)
		restrictflags = ca_all;
	else
		restrictflags = allow_flags(mc, restrictflags);
	oldflags = get_template_flags(mc, role);

	if (oldflags == 0)
	{
		command_fail(si, fault_nosuch_target, _("Role \2%s\2 does not exist."), role);
		return;
	}

	newflags = xflag_apply_batch(oldflags, parc - 2, parv + 2);
	if ((oldflags | newflags) & ~restrictflags)
	{
		unsigned int delta = (oldflags | newflags) & ~restrictflags;

		command_fail(si, fault_badparams, _("You do not have appropriate permissions to set flags: \2%s\2"), xflag_tostr(delta));
		return;
	}

	if ((oldflags ^ newflags) & CA_FOUNDER)
	{
		command_fail(si, fault_unimplemented, _("Adding or removing founder status from a role is not implemented."));
		return;
	}

	if (newflags & CA_FOUNDER)
		newflags |= CA_FLAGS;

	if (newflags == 0)
	{
		if (oldflags == 0)
			command_fail(si, fault_badparams, _("No valid flags given, use /%s%s HELP ROLE ADD for a list"), ircd->uses_rcommand ? "" : "msg ", chansvs.me->disp);
		else
			command_fail(si, fault_nosuch_target, _("You cannot remove all flags from the role \2%s\2."), role);
		return;
	}
	l = build_template_list(mc);
	if (l != NULL)
	{
		mowgli_node_t *n;

		MOWGLI_ITER_FOREACH(n, l->head)
		{
			template_t *t = n->data;

			if (t->level == newflags)
			{
				command_fail(si, fault_alreadyexists, _("The role \2%s\2 already has flags \2%s\2."), t->name, xflag_tostr(newflags));
				return;
			}
		}

		free_template_list(l);
	}

	command_success_nodata(si, _("Flags for role \2%s\2 were changed to: \2%s\2."), role, xflag_tostr(newflags));
	update_role_entry(si, mc, role, newflags);
}

/*
 * Syntax: ROLE #channel DEL <role>
 *
 * Output:
 *
 * Deletes a role.
 */
static void cs_cmd_role_del(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	const char *channel = parv[0];
	const char *role = parv[1];
	unsigned int oldflags, restrictflags;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!role)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ROLE DEL");
		command_fail(si, fault_needmoreparams, _("Syntax: ROLE <#channel> DEL <role>"));
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_FLAGS))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	restrictflags = chanacs_source_flags(mc, si);
	if (restrictflags & CA_FOUNDER)
		restrictflags = ca_all;
	else
		restrictflags = allow_flags(mc, restrictflags);
	oldflags = get_template_flags(mc, role);
	if (oldflags == 0)
	{
		command_fail(si, fault_toomany, _("Role \2%s\2 does not exist."), role);
		return;
	}

	if (oldflags & ~restrictflags)
	{
		unsigned int delta = oldflags & ~restrictflags;

		command_fail(si, fault_badparams, _("You do not have appropriate permissions to set flags: \2%s\2"), xflag_tostr(delta));
		return;
	}

	command_success_nodata(si, _("Role \2%s\2 has been deleted."), role);
	update_role_entry(si, mc, role, 0);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
