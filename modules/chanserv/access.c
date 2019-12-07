/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010-2011 William Pitcock <nenolod@atheme.org>
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * ACCESS command implementation for ChanServ.
 */

#include <atheme.h>

struct channel_template
{
	char name[400];
	unsigned int level;
	mowgli_node_t node;
};

struct channel_template_iter
{
	struct mychan *mc;
	mowgli_list_t *l;
};

static mowgli_patricia_t *cs_access_cmds = NULL;
static mowgli_patricia_t *cs_role_cmds = NULL;

static void
cs_help_wrapper(struct sourceinfo *const restrict si, const char *const restrict subcmd,
                const char *const restrict cmdname, mowgli_patricia_t *const restrict cmdlist)
{
	if (subcmd)
	{
		(void) help_display_as_subcmd(si, chansvs.me, cmdname, subcmd, cmdlist);
		return;
	}

	(void) help_display_prefix(si, chansvs.me);
	(void) command_success_nodata(si, _("Help for \2%s\2:"), cmdname);
	(void) help_display_newline(si);
	(void) command_help(si, cmdlist);
	(void) help_display_moreinfo(si, chansvs.me, cmdname);
	(void) help_display_suffix(si);
}

static void
cs_cmd_wrapper(struct sourceinfo *const restrict si, const int parc, char **const restrict parv,
               const char *const restrict cmdname, mowgli_patricia_t *const restrict cmdlist)
{
	if (parc < 2)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, cmdname);
		(void) command_fail(si, fault_needmoreparams, _("Syntax: %s <#channel> <command> [parameters]"),
		                    cmdname);
		return;
	}

	char *subcmd;
	char *target;

	if (parv[0][0] == '#')
	{
		subcmd = parv[1];
		target = parv[0];
	}
	else if (parv[1][0] == '#')
	{
		subcmd = parv[0];
		target = parv[1];
	}
	else
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, cmdname);
		(void) command_fail(si, fault_badparams, _("Syntax: %s <#channel> <command> [parameters]"), cmdname);
		return;
	}

	const struct command *const cmd = command_find(cmdlist, subcmd);

	if (! cmd)
	{
		(void) help_display_invalid(si, chansvs.me, cmdname);
		return;
	}

	char buf[BUFSIZE];

	if (parc > 2)
		(void) snprintf(buf, sizeof buf, "%s %s", target, parv[2]);
	else
		(void) mowgli_strlcpy(buf, target, sizeof buf);

	(void) command_exec_split(chansvs.me, si, cmd->name, buf, cmdlist);
}

static void
cs_help_access(struct sourceinfo *const restrict si, const char *const restrict subcmd)
{
	(void) cs_help_wrapper(si, subcmd, "ACCESS", cs_access_cmds);
}

static void
cs_help_role(struct sourceinfo *const restrict si, const char *const restrict subcmd)
{
	(void) cs_help_wrapper(si, subcmd, "ROLE", cs_role_cmds);
}

static void
cs_cmd_access(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	(void) cs_cmd_wrapper(si, parc, parv, "ACCESS", cs_access_cmds);
}

static void
cs_cmd_role(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	(void) cs_cmd_wrapper(si, parc, parv, "ROLE", cs_role_cmds);
}

static inline unsigned int
count_bits(unsigned int bits)
{
	unsigned int i = 1, count = 0;

	for (i = 1; i < sizeof(bits) * 8; i++)
	{
		if (bits & (1 << i))
			count++;
	}

	return count;
}

static int
compare_template_nodes(mowgli_node_t *a, mowgli_node_t *b, void *opaque)
{
	struct channel_template *ta = a->data;
	struct channel_template *tb = b->data;

	return count_bits(ta->level) - count_bits(tb->level);
}

static struct channel_template *
find_template(mowgli_list_t *l, const char *key)
{
	mowgli_node_t *iter;

	return_val_if_fail(l != NULL, NULL);
	return_val_if_fail(key != NULL, NULL);

	MOWGLI_ITER_FOREACH(iter, l->head)
	{
		struct channel_template *t = iter->data;

		if (!strcasecmp(t->name, key))
			return t;
	}

	return NULL;
}

static int
append_global_template(const char *key, void *data, void *privdata)
{
	struct channel_template *t;
	struct channel_template_iter *ti = privdata;
	struct default_template *def_t = data;
	unsigned int vopflags;

	if (!chansvs.hide_xop)
	{
		vopflags = get_global_template_flags("VOP");

		if (def_t->flags == vopflags && !strcasecmp(key, "HOP"))
			return 0;
	}

	if ((t = find_template(ti->l, key)) != NULL)
		return 0;

	t = smalloc(sizeof *t);
	mowgli_strlcpy(t->name, key, sizeof(t->name));
	t->level = get_template_flags(ti->mc, key);
	mowgli_node_add(t, &t->node, ti->l);

        return 0;
}

static mowgli_list_t *
build_template_list(struct mychan *mc)
{
	const char *p, *q, *r;
	char *s;
	char ss[40];
	static char flagname[400];
	struct metadata *md;
	mowgli_list_t *l;
	struct channel_template *t;
	struct channel_template_iter ti;

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

			t = smalloc(sizeof *t);
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

	// reverse the list so that we go from highest flags to lowest.
	mowgli_list_reverse(l);

	return l;
}

static void
free_template_list(mowgli_list_t *l)
{
	mowgli_node_t *n, *tn;

	return_if_fail(l != NULL);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, l->head)
	{
		struct channel_template *t = n->data;

		mowgli_node_delete(&t->node, l);
		sfree(t);
	}
}

/* get_template_name()
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
static const char *
get_template_name(struct mychan *mc, unsigned int level)
{
	mowgli_list_t *l;
	mowgli_node_t *n;
	static char flagname[400];
	struct channel_template *exact_t = NULL;

	l = build_template_list(mc);

	// find exact_t, lesser_t and greater_t
	MOWGLI_ITER_FOREACH(n, l->head)
	{
		struct channel_template *t = n->data;

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

// Update a role entry and synchronize the changes with the access list.
static void
update_role_entry(struct sourceinfo *si, struct mychan *mc, const char *role, unsigned int flags)
{
	struct metadata *md;
	size_t l;
	char *p, *q, *r;
	char ss[40], newstr[400];
	bool found = false;
	unsigned int oldflags;
	char *flagstr;
	mowgli_node_t *n, *tn;
	struct chanacs *ca;
	unsigned int changes = 0;
	struct hook_channel_acl_req req;

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
						// otherwise, zap the space before it
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

			// don't change foundership status.
			if ((oldflags ^ flags) & CA_FOUNDER)
				continue;

			req.ca = ca;
			req.oldlevel = ca->level;

			changes++;
			chanacs_modify_simple(ca, flags, ~flags, si->smu);

			req.newlevel = ca->level;

			hook_call_channel_acl_change(&req);
			chanacs_close(ca);
		}
	}

	logcommand(si, CMDLOG_SET, "ROLE:MOD: \2%s\2 \2%s\2 !\2%s\2 (\2%u\2 changes)", mc->name, role, flagstr, changes);

	if (changes > 0)
		command_success_nodata(si, ngettext(N_("%u access entry updated accordingly."),
		                                    N_("%u access entries updated accordingly."),
		                                    changes), changes);
}

static unsigned int
xflag_apply_batch(unsigned int in, int parc, char *parv[])
{
	unsigned int out;
	int i;

	out = in;
	for (i = 0; i < parc; i++)
		out = xflag_apply(out, parv[i]);

	return out;
}

/* Syntax: ACCESS #channel LIST
 *
 * Output:
 *
 * Entry Nickname/Host          Role
 * ----- ---------------------- ----
 * 1     nenolod                founder
 * 2     jdhore                 channel-staffer
 */
static void
cs_cmd_access_list(struct sourceinfo *si, int parc, char *parv[])
{
	struct chanacs *ca;
	mowgli_node_t *n;
	struct mychan *mc;
	const char *channel = parv[0];
	bool operoverride = false;
	unsigned int i = 1;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
		return;
	}

	if (!(mc->flags & MC_PUBACL) && !chanacs_source_has_flag(mc, si, CA_ACLVIEW))
	{
		if (has_priv(si, PRIV_CHAN_AUSPEX))
			operoverride = true;
		else
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}
	}

	/* TRANSLATORS: Adjust these numbers only if the translated column
	 * headers would exceed that length. Pay particular attention to
	 * also changing the numbers in the format string inside the loop
	 * below to match them, and beware that these format strings are
	 * shared across multiple files!
	 */
	command_success_nodata(si, _("%-8s %-22s %s"), _("Entry"), _("Nickname/Host"), _("Role"));
	command_success_nodata(si, "----------------------------------------------------------------");

	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		const char *role;

		ca = n->data;

		if (ca->level == CA_AKICK)
			continue;

		role = get_template_name(mc, ca->level);

		command_success_nodata(si, _("%-8u %-22s %s"), i, ca->entity ? ca->entity->name : ca->host, role);

		i++;
	}

	command_success_nodata(si, "----------------------------------------------------------------");
	command_success_nodata(si, _("End of \2%s\2 ACCESS listing."), channel);

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "ACCESS:LIST: \2%s\2 (oper override)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "ACCESS:LIST: \2%s\2", mc->name);
}

/* Syntax: ACCESS #channel INFO user
 *
 * Output:
 *
 * Access for nenolod in #atheme:
 * Role       : Founder
 * Flags      : aclview, ...
 * Modified   : Aug 18 21:41:31 2005 (5 years, 6 weeks, 0 days, 05:57:21 ago)
 * *** End of Info ***
 */
static void
cs_cmd_access_info(struct sourceinfo *si, int parc, char *parv[])
{
	struct chanacs *ca;
	struct myentity *mt;
	struct myentity *setter;
	struct mychan *mc;
	const char *channel = parv[0];
	const char *target = parv[1];
	bool operoverride = false;
	const char *role;
	const char *setter_name;
	struct tm *tm;
	char strfbuf[BUFSIZE];
	struct metadata *md;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
		return;
	}

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS INFO");
		command_fail(si, fault_needmoreparams, _("Syntax: ACCESS <#channel> INFO <account|hostmask>"));
		return;
	}

	if (!(mc->flags & MC_PUBACL) && !chanacs_source_has_flag(mc, si, CA_ACLVIEW))
	{
		if (has_priv(si, PRIV_CHAN_AUSPEX))
			operoverride = true;
		else
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}
	}

	if (validhostmask(target))
		ca = chanacs_find_host_literal(mc, target, 0);
	else
	{
		if (!(mt = myentity_find_ext(target)))
		{
			command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
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

	// Taken from chanserv/flags.c
	if (*ca->setter_uid != '\0' && (setter = myentity_find_uid(ca->setter_uid)))
		setter_name = setter->name;
	else
		setter_name = "?";

	tm = localtime(&ca->tmodified);
	strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

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
	command_success_nodata(si, _("Modified   : %s (%s ago) by %s"), strfbuf, time_ago(ca->tmodified), setter_name);
	command_success_nodata(si, _("*** \2End of Info\2 ***"));

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "ACCESS:INFO: \2%s\2 on \2%s\2 (oper override)", mc->name, target);
	else
		logcommand(si, CMDLOG_GET, "ACCESS:INFO: \2%s\2 on \2%s\2", mc->name, target);
}

/* Syntax: ACCESS #channel DEL user
 *
 * Output:
 *
 * nenolod was removed from the Founder role in #atheme.
 */
static void
cs_cmd_access_del(struct sourceinfo *si, int parc, char *parv[])
{
	struct chanacs *ca;
	struct myentity *mt;
	struct mychan *mc;
	struct hook_channel_acl_req req;
	unsigned int restrictflags;
	const char *channel = parv[0];
	const char *target = parv[1];
	const char *role;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
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
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (validhostmask(target))
		ca = chanacs_open(mc, NULL, target, true, entity(si->smu));
	else
	{
		if (mt == NULL)
		{
			command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
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
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}

		if (entity(si->smu) != mt)
			restrictflags = allow_flags(mc, restrictflags);
	}

	if (!chanacs_modify(ca, &req.newlevel, &req.oldlevel, restrictflags, si->smu))
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

/* Syntax: ACCESS #channel ADD user role
 *
 * Output:
 *
 * nenolod was added with the Founder role in #atheme.
 */
static void
cs_cmd_access_add(struct sourceinfo *si, int parc, char *parv[])
{
	struct chanacs *ca;
	struct myentity *mt = NULL;
	struct mychan *mc;
	struct hook_channel_acl_req req;
	unsigned int oldflags, restrictflags;
	unsigned int newflags, addflags, removeflags;
	const char *channel = parv[0];
	const char *target = parv[1];
	const char *role = parv[2];

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
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
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (validhostmask(target))
		ca = chanacs_open(mc, NULL, target, true, entity(si->smu));
	else
	{
		if (!(mt = myentity_find_ext(target)))
		{
			command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
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
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
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
			command_fail(si, fault_noprivs, ngettext(N_("Only %u founder allowed per channel."),
			                                         N_("Only %u founders allowed per channel."),
			                                         chansvs.maxfounders), chansvs.maxfounders);
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
		if (!myentity_allow_foundership(mt))
		{
			command_fail(si, fault_toomany, _("\2%s\2 cannot take foundership of a channel."), mt->name);
			chanacs_close(ca);
			return;
		}
	}

	oldflags = ca->level;

	addflags = newflags & ~oldflags;
	removeflags = ca_all & ~newflags;

	if (!chanacs_modify(ca, &addflags, &removeflags, restrictflags, si->smu))
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

/* Syntax: ACCESS #channel SET user role
 *
 * Output:
 *
 * nenolod now has the Founder role in #atheme.
 */
static void
cs_cmd_access_set(struct sourceinfo *si, int parc, char *parv[])
{
	struct chanacs *ca;
	struct myentity *mt = NULL;
	struct mychan *mc;
	struct hook_channel_acl_req req;
	unsigned int oldflags, restrictflags;
	unsigned int newflags, addflags, removeflags;
	const char *channel = parv[0];
	const char *target = parv[1];
	const char *role = parv[2];

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
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
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (validhostmask(target))
		ca = chanacs_open(mc, NULL, target, true, entity(si->smu));
	else
	{
		if (!(mt = myentity_find_ext(target)))
		{
			command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
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
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
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
			command_fail(si, fault_noprivs, ngettext(N_("Only %u founder allowed per channel."),
			                                         N_("Only %u founders allowed per channel."),
			                                         chansvs.maxfounders), chansvs.maxfounders);
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

	if (!chanacs_modify(ca, &addflags, &removeflags, restrictflags, si->smu))
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

/* Syntax: ROLE #channel LIST
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
static void
cs_cmd_role_list(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	const char *channel = parv[0];
	mowgli_list_t *l;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
		return;
	}

	l = build_template_list(mc);
	if (l != NULL)
	{
		mowgli_node_t *n;

		command_success_nodata(si, _("List of channel-defined roles:"));

		MOWGLI_ITER_FOREACH(n, l->head)
		{
			struct channel_template *t = n->data;

			command_success_nodata(si, "%-20s: %s (%s)", t->name, xflag_tostr(t->level), bitmask_to_flags(t->level));
		}

		free_template_list(l);
	}
}

/* Syntax: ROLE #channel ADD <role> [flags-changes]
 *
 * Output:
 *
 * Creates a new role with the given flags.
 */
static void
cs_cmd_role_add(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	mowgli_list_t *l;
	const char *channel = parv[0];
	const char *role = parv[1];
	unsigned int oldflags, newflags, restrictflags;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
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
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
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
			command_fail(si, fault_badparams, _("No valid flags given, use \2/msg %s HELP ROLE ADD\2 "
			                                    "for a list"), chansvs.me->disp);
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
			struct channel_template *t = n->data;

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

/* Syntax: ROLE #channel SET <role> [flags-changes]
 *
 * Output:
 *
 * Creates a new role with the given flags.
 */
static void
cs_cmd_role_set(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	mowgli_list_t *l;
	const char *channel = parv[0];
	const char *role = parv[1];
	unsigned int oldflags, newflags, restrictflags;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
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
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
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
			command_fail(si, fault_badparams, _("No valid flags given, use \2/msg %s HELP ROLE ADD\2 "
			                                    "for a list"), chansvs.me->disp);
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
			struct channel_template *t = n->data;

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

/* Syntax: ROLE #channel DEL <role>
 *
 * Output:
 *
 * Deletes a role.
 */
static void
cs_cmd_role_del(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	const char *channel = parv[0];
	const char *role = parv[1];
	unsigned int oldflags, restrictflags;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
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
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
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

static struct command cs_access = {
	.name           = "ACCESS",
	.desc           = N_("Manage channel access."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_access,
	.help           = { .func = &cs_help_access },
};

static struct command cs_role = {
	.name           = "ROLE",
	.desc           = N_("Manage channel roles."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_role,
	.help           = { .func = &cs_help_role },
};

static struct command cs_access_list = {
	.name           = "LIST",
	.desc           = N_("List channel access entries."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &cs_cmd_access_list,
	.help           = { .path = "cservice/access_list" },
};

static struct command cs_access_info = {
	.name           = "INFO",
	.desc           = N_("Display information on an access list entry."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_access_info,
	.help           = { .path = "cservice/access_info" },
};

static struct command cs_access_del = {
	.name           = "DEL",
	.desc           = N_("Delete an access list entry."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_access_del,
	.help           = { .path = "cservice/access_del" },
};

static struct command cs_access_add = {
	.name           = "ADD",
	.desc           = N_("Add an access list entry."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_access_add,
	.help           = { .path = "cservice/access_add" },
};

static struct command cs_access_set = {
	.name           = "SET",
	.desc           = N_("Update an access list entry."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_access_set,
	.help           = { .path = "cservice/access_set" },
};

static struct command cs_role_list = {
	.name           = "LIST",
	.desc           = N_("List available roles."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &cs_cmd_role_list,
	.help           = { .path = "cservice/role_list" },
};

static struct command cs_role_add = {
	.name           = "ADD",
	.desc           = N_("Add a role."),
	.access         = AC_NONE,
	.maxparc        = 20,
	.cmd            = &cs_cmd_role_add,
	.help           = { .path = "cservice/role_add" },
};

static struct command cs_role_set = {
	.name           = "SET",
	.desc           = N_("Change flags on a role."),
	.access         = AC_NONE,
	.maxparc        = 20,
	.cmd            = &cs_cmd_role_set,
	.help           = { .path = "cservice/role_set" },
};

static struct command cs_role_del = {
	.name           = "DEL",
	.desc           = N_("Delete a role."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_role_del,
	.help           = { .path = "cservice/role_del" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	if (! (cs_access_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	if (! (cs_role_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		(void) mowgli_patricia_destroy(cs_access_cmds, NULL, NULL);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&cs_access_list, cs_access_cmds);
	(void) command_add(&cs_access_info, cs_access_cmds);
	(void) command_add(&cs_access_del, cs_access_cmds);
	(void) command_add(&cs_access_add, cs_access_cmds);
	(void) command_add(&cs_access_set, cs_access_cmds);

	(void) command_add(&cs_role_list, cs_role_cmds);
	(void) command_add(&cs_role_add, cs_role_cmds);
	(void) command_add(&cs_role_set, cs_role_cmds);
	(void) command_add(&cs_role_del, cs_role_cmds);

	(void) service_named_bind_command("chanserv", &cs_access);
	(void) service_named_bind_command("chanserv", &cs_role);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("chanserv", &cs_access);
	(void) service_named_unbind_command("chanserv", &cs_role);

	(void) mowgli_patricia_destroy(cs_access_cmds, &command_delete_trie_cb, cs_access_cmds);
	(void) mowgli_patricia_destroy(cs_role_cmds, &command_delete_trie_cb, cs_role_cmds);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/access", MODULE_UNLOAD_CAPABILITY_OK)
