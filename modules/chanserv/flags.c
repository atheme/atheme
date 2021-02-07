/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 William Pitcock, et al.
 *
 * This file contains code for the CService FLAGS functions.
 */

#include <atheme.h>

struct template_iter
{
	const char *res;
	unsigned int level;
};

static bool anope_flags_compat = true;

static int
global_template_search(const char *key, void *data, void *privdata)
{
	struct template_iter *iter = privdata;
	struct default_template *def_t = data;

	if (def_t->flags == iter->level)
		iter->res = key;

	return 0;
}

static const char *
get_template_name(struct mychan *mc, unsigned int level)
{
	struct metadata *md;
	const char *p, *q, *r;
	char *s;
	char ss[40];
	static char flagname[400];
	struct template_iter iter;

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
			if (level == flags_to_bitmask(ss, 0))
			{
				mowgli_strlcpy(flagname, p, sizeof flagname);
				s = strchr(flagname, '=');
				if (s != NULL)
					*s = '\0';
				return flagname;
			}
			p = r;
		}
	}

	iter.res = NULL;
	iter.level = level;
	mowgli_patricia_foreach(global_template_dict, global_template_search, &iter);

	return iter.res;
}

static void
do_list(struct sourceinfo *si, struct mychan *mc, unsigned int flags)
{
	struct chanacs *ca;
	mowgli_node_t *n;
	bool operoverride = false;
	unsigned int i = 1;

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

	bool show_akicks = true;

	if (chansvs.hide_pubacl_akicks)
		show_akicks = ( chanacs_source_has_flag(mc, si, CA_ACLVIEW) || has_priv(si, PRIV_CHAN_AUSPEX) );

	/* TRANSLATORS: Adjust these numbers only if the translated column
	 * headers would exceed that length. Pay particular attention to
	 * also changing the numbers in the format string inside the loop
	 * below to match them, and beware that these format strings are
	 * shared across multiple files!
	 */
	command_success_nodata(si, _("%-8s %-22s %s"), _("Entry"), _("Nickname/Host"), _("Flags"));
	command_success_nodata(si, "----------------------------------------------------------------");

	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		const char *template, *mod_ago;
		struct tm *tm;
		char mod_date[64];
		struct myentity *setter;
		const char *setter_name;

		ca = n->data;

		if (ca->level == CA_AKICK && !show_akicks)
			continue;

		if (flags && !(ca->level & flags))
			continue;

		template = get_template_name(mc, ca->level);
		mod_ago = ca->tmodified ? time_ago(ca->tmodified) : "?";

		tm = localtime(&ca->tmodified);
		strftime(mod_date, sizeof mod_date, TIME_FORMAT, tm);

		if (*ca->setter_uid != '\0' && (setter = myentity_find_uid(ca->setter_uid)))
			setter_name = setter->name;
		else
			setter_name = "?";

		if (template != NULL)
			command_success_nodata(si, _("%-8u %-22s %-20s (%s) [modified %s ago, on %s, by %s]"),
				i, ca->entity ? ca->entity->name : ca->host, bitmask_to_flags(ca->level), template, mod_ago, mod_date, setter_name);
		else
			command_success_nodata(si, _("%-8u %-22s %-20s [modified %s ago, on %s, by %s]"),
				i, ca->entity ? ca->entity->name : ca->host, bitmask_to_flags(ca->level), mod_ago, mod_date, setter_name);
		i++;
	}

	command_success_nodata(si, "----------------------------------------------------------------");
	command_success_nodata(si, _("End of \2%s\2 FLAGS listing."), mc->name);

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "FLAGS: \2%s\2 (oper override)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "FLAGS: \2%s\2", mc->name);
}

static void
check_registration_keywords(struct hook_user_register_check *hdata)
{
	if (hdata->approved || !anope_flags_compat)
	{
		return;
	}

	if (!strcasecmp(hdata->account, "LIST") || !strcasecmp(hdata->account, "CLEAR") || !strcasecmp(hdata->account, "MODIFY"))
	{
		command_fail(hdata->si, fault_badparams, _("The nickname \2%s\2 is reserved and cannot be registered."), hdata->account);
		hdata->approved = 1;
	}
}

// FLAGS <channel> [user] [flags]
static void
cs_cmd_flags(struct sourceinfo *si, int parc, char *parv[])
{
	struct chanacs *ca;
	mowgli_node_t *n;
	char *channel = parv[0];
	char *target = sstrdup(parv[1]);
	char *flagstr = parv[2];
	const char *str1;
	unsigned int addflags, removeflags, restrictflags;
	struct hook_channel_acl_req req;
	struct mychan *mc;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FLAGS");
		command_fail(si, fault_needmoreparams, _("Syntax: FLAGS <channel> [target] [flags]"));
		sfree(target);
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
		sfree(target);
		return;
	}

	if (metadata_find(mc, "private:close:closer") && (target || !has_priv(si, PRIV_CHAN_AUSPEX)))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, channel);
		sfree(target);
		return;
	}

	if (!target || (target && target[0] == '+' && flagstr == NULL))
	{
		unsigned int flags = (target != NULL) ? flags_to_bitmask(target, 0) : 0;

		do_list(si, mc, flags);
		sfree(target);
		return;
	}

	/*
	 * following conditions are for compatibility with Anope just to avoid a whole clusterfuck
	 * of confused users caused by their 'innovation.'  yeah, that's a word for it alright.
	 *
	 * anope 1.9's shiny new FLAGS command has:
	 *
	 * FLAGS #channel LIST
	 * FLAGS #channel MODIFY user flagspec
	 * FLAGS #channel CLEAR
	 *
	 * obviously they do not support the atheme syntax, because lets face it, they like to
	 * 'innovate.'  this is, of course, hilarious for obvious reasons.  never mind that we
	 * *invented* the FLAGS system for channel ACLs, so you would think they would find it
	 * worthwhile to be compatible here.  i guess that would have been too obvious or something
	 * about their whole 'stealing our design' thing that they have been doing in 1.9 since the
	 * beginning...  or do i mean 'innovating?'
	 *
	 * anyway we rewrite the commands as appropriate in the two if blocks below so that they
	 * are processed by the flags code as the user would intend.  obviously, we're not really
	 * capable of handling the anope flag model (which makes honestly zero sense to me, and is
	 * extremely complex which kind of misses the entire point of the flags UI design...) so if
	 * some user tries passing anope flags, it will probably be hilarious.  the good news is
	 * most of the anope flags tie up to atheme flags in some weird way anyway (probably because,
	 * i don't know, they copied the entire design and then fucked it up?  yeah.  probably that.)
	 *
	 *   --nenolod
	 */
	else if (anope_flags_compat && !strcasecmp(target, "LIST") && myentity_find_ext(target) == NULL)
	{
		do_list(si, mc, 0);
		sfree(target);
		return;
	}
	else if (anope_flags_compat && !strcasecmp(target, "CLEAR") && myentity_find_ext(target) == NULL)
	{
		sfree(target);

		if (!chanacs_source_has_flag(mc, si, CA_FOUNDER))
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}

		mowgli_node_t *tn;

		MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chanacs.head)
		{
			ca = n->data;

			if (ca->level & CA_FOUNDER)
				continue;

			atheme_object_unref(ca);
		}

		logcommand(si, CMDLOG_DO, "CLEAR:FLAGS: \2%s\2", mc->name);
		command_success_nodata(si, _("Cleared flags in \2%s\2."), mc->name);
		return;
	}
	else if (anope_flags_compat && !strcasecmp(target, "MODIFY") && myentity_find_ext(target) == NULL)
	{
		sfree(target);

		if (parc < 3)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FLAGS");
			command_fail(si, fault_needmoreparams, _("Syntax: FLAGS <#channel> MODIFY [target] <flags>"));
			return;
		}

		flagstr = strchr(parv[2], ' ');
		if (flagstr)
			*flagstr++ = '\0';

		target = sstrdup(parv[2]);
	}

	{
		struct myentity *mt;

		if (!si->smu)
		{
			command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
			sfree(target);
			return;
		}

		if (!flagstr)
		{
			if (!(mc->flags & MC_PUBACL) && !chanacs_source_has_flag(mc, si, CA_ACLVIEW))
			{
				command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
				sfree(target);
				return;
			}

			bool show_akicks = true;

			if (chansvs.hide_pubacl_akicks)
				show_akicks = chanacs_source_has_flag(mc, si, CA_ACLVIEW);

			if (validhostmask(target))
				ca = chanacs_find_host_literal(mc, target, 0);
			else
			{
				if (!(mt = myentity_find_ext(target)))
				{
					command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
					sfree(target);
					return;
				}
				sfree(target);
				target = sstrdup(mt->name);
				ca = chanacs_find_literal(mc, mt, 0);
			}
			if (ca != NULL && (ca->level != CA_AKICK || show_akicks))
			{
				str1 = bitmask_to_flags2(ca->level, 0);
				command_success_string(si, str1, _("Flags for \2%s\2 in \2%s\2 are \2%s\2."),
						target, channel,
						str1);
			}
			else
				command_success_string(si, "", _("No flags for \2%s\2 in \2%s\2."),
						target, channel);
			logcommand(si, CMDLOG_GET, "FLAGS: \2%s\2 on \2%s\2", mc->name, target);
			sfree(target);
			return;
		}

		// founder may always set flags -- jilles
		restrictflags = chanacs_source_flags(mc, si);
		if (restrictflags & CA_FOUNDER)
			restrictflags = ca_all;
		else
		{
			if (!(restrictflags & CA_FLAGS))
			{
				/* allow a user to remove their own access
				 * even without +f */
				if (restrictflags & CA_AKICK ||
						si->smu == NULL ||
						irccasecmp(target, entity(si->smu)->name) ||
						strcmp(flagstr, "-*"))
				{
					command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
					sfree(target);
					return;
				}
			}
			if (irccasecmp(target, entity(si->smu)->name))
				restrictflags = allow_flags(mc, restrictflags);
			else
				restrictflags |= allow_flags(mc, restrictflags);
		}

		if (*flagstr == '+' || *flagstr == '-' || *flagstr == '=')
		{
			flags_make_bitmasks(flagstr, &addflags, &removeflags);
			if (addflags == 0 && removeflags == 0)
			{
				command_fail(si, fault_badparams, _("No valid flags given, use \2/msg %s HELP FLAGS\2 "
				                                    "for a list"), chansvs.me->disp);
				sfree(target);
				return;
			}
		}
		else
		{
			addflags = get_template_flags(mc, flagstr);
			if (addflags == 0)
			{
				// Hack -- jilles
				if (*target == '+' || *target == '-' || *target == '=')
					command_fail(si, fault_badparams, _("Usage: FLAGS %s [target] [flags]"), mc->name);
				else
					command_fail(si, fault_badparams, _("Invalid template name given, use \2/msg "
					                                    "%s TEMPLATE %s\2 for a list"),
					                                    chansvs.me->disp, mc->name);
				sfree(target);
				return;
			}
			removeflags = ca_all & ~addflags;
		}

		if (!validhostmask(target))
		{
			if (!(mt = myentity_find_ext(target)))
			{
				command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
				sfree(target);
				return;
			}
			sfree(target);
			target = sstrdup(mt->name);

			ca = chanacs_open(mc, mt, NULL, true, entity(si->smu));

			if (ca->level & CA_FOUNDER && removeflags & CA_FLAGS && !(removeflags & CA_FOUNDER))
			{
				command_fail(si, fault_noprivs, _("You may not remove a founder's +f access."));
				sfree(target);
				return;
			}
			if (ca->level & CA_FOUNDER && removeflags & CA_FOUNDER && mychan_num_founders(mc) == 1)
			{
				command_fail(si, fault_noprivs, _("You may not remove the last founder."));
				sfree(target);
				return;
			}
			if (!(ca->level & CA_FOUNDER) && addflags & CA_FOUNDER)
			{
				if (mychan_num_founders(mc) >= chansvs.maxfounders)
				{
					command_fail(si, fault_noprivs, ngettext(N_("Only %u founder allowed per channel."),
					                                         N_("Only %u founders allowed per channel."),
					                                         chansvs.maxfounders), chansvs.maxfounders);
					chanacs_close(ca);
					sfree(target);
					return;
				}
				if (!myentity_can_register_channel(mt))
				{
					command_fail(si, fault_toomany, _("\2%s\2 has too many channels registered."), mt->name);
					chanacs_close(ca);
					sfree(target);
					return;
				}
				if (!myentity_allow_foundership(mt))
				{
					command_fail(si, fault_toomany, _("\2%s\2 cannot take foundership of a channel."), mt->name);
					chanacs_close(ca);
					sfree(target);
					return;
				}
			}
			if (addflags & CA_FOUNDER)
			{
				addflags |= CA_FLAGS;
				removeflags &= ~CA_FLAGS;
			}

			/* If NEVEROP is set, don't allow adding new entries
			 * except sole +b. Adding flags if the current level
			 * is +b counts as adding an entry.
			 * -- jilles */
			/* XXX: not all entities are users */
			if (isuser(mt) && (MU_NEVEROP & user(mt)->flags && addflags != CA_AKICK && addflags != 0 && (ca->level == 0 || ca->level == CA_AKICK)))
			{
				command_fail(si, fault_noprivs, _("\2%s\2 does not wish to be added to channel access lists (NEVEROP set)."), mt->name);
				chanacs_close(ca);
				sfree(target);
				return;
			}

			if (ca->level == 0 && chanacs_is_table_full(ca))
			{
				command_fail(si, fault_toomany, _("Channel \2%s\2 access list is full."), mc->name);
				chanacs_close(ca);
				sfree(target);
				return;
			}

			req.ca = ca;
			req.oldlevel = ca->level;

			if (!chanacs_modify(ca, &addflags, &removeflags, restrictflags, si->smu))
			{
				command_fail(si, fault_noprivs, _("You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2."), bitmask_to_flags2(addflags, removeflags), mt->name, mc->name);
				chanacs_close(ca);
				sfree(target);
				return;
			}

			req.newlevel = ca->level;

			hook_call_channel_acl_change(&req);
			chanacs_close(ca);
		}
		else
		{
			if (addflags & CA_FOUNDER)
			{
		                command_fail(si, fault_badparams, _("You may not set founder status on a hostmask."));
				sfree(target);
				return;
			}

			ca = chanacs_open(mc, NULL, target, true, entity(si->smu));
			if (ca->level == 0 && chanacs_is_table_full(ca))
			{
				command_fail(si, fault_toomany, _("Channel \2%s\2 access list is full."), mc->name);
				chanacs_close(ca);
				sfree(target);
				return;
			}

			req.ca = ca;
			req.oldlevel = ca->level;

			if (!chanacs_modify(ca, &addflags, &removeflags, restrictflags, si->smu))
			{
		                command_fail(si, fault_noprivs, _("You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2."), bitmask_to_flags2(addflags, removeflags), target, mc->name);
				chanacs_close(ca);
				sfree(target);
				return;
			}

			req.newlevel = ca->level;

			hook_call_channel_acl_change(&req);
			chanacs_close(ca);
		}

		if ((addflags | removeflags) == 0)
		{
			command_fail(si, fault_nochange, _("Channel access to \2%s\2 for \2%s\2 unchanged."), channel, target);
			sfree(target);
			return;
		}

		flagstr = bitmask_to_flags2(addflags, removeflags);
		command_success_nodata(si, _("Flags \2%s\2 were set on \2%s\2 in \2%s\2."), flagstr, target, channel);
		logcommand(si, CMDLOG_SET, "FLAGS: \2%s\2 \2%s\2 \2%s\2", mc->name, target, flagstr);
		verbose(mc, "\2%s\2 set flags \2%s\2 on \2%s\2", get_source_name(si), flagstr, target);
	}

	sfree(target);
}

static struct command cs_flags = {
	.name           = "FLAGS",
	.desc           = N_("Manipulates specific permissions on a channel."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_flags,
	.help           = { .path = "cservice/flags" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	service_named_bind_command("chanserv", &cs_flags);

	add_bool_conf_item("ANOPE_FLAGS_COMPAT", &chansvs.me->conf_table, 0, &anope_flags_compat, true);

	hook_add_nick_can_register(check_registration_keywords);
	hook_add_user_can_register(check_registration_keywords);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_flags);

	hook_del_nick_can_register(check_registration_keywords);

	hook_del_user_can_register(check_registration_keywords);

	del_conf_item("ANOPE_FLAGS_COMPAT", &chansvs.me->conf_table);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/flags", MODULE_UNLOAD_CAPABILITY_OK)
