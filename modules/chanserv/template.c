/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Jilles Tjoelker, et al.
 *
 * This file contains code for the CService TEMPLATE functions.
 */

#include <atheme.h>

static int
display_template(const char *key, void *data, void *privdata)
{
	struct sourceinfo *si = privdata;
	struct default_template *def_t = data;
	unsigned int vopflags;

	vopflags = get_global_template_flags("VOP");
	if (def_t->flags == vopflags && !strcasecmp(key, "HOP"))
		return 0;

	command_success_nodata(si, _("%-20s %s"), key, bitmask_to_flags(def_t->flags));

	return 0;
}

static void
list_generic_flags(struct sourceinfo *si)
{
	/* TRANSLATORS: Adjust these numbers only if the translated column
	 * headers would exceed that length. Pay particular attention to
	 * also changing the numbers in the format string inside the function
	 * above to match them!
	 */
	command_success_nodata(si, _("%-20s %s"), _("Name"), _("Flags"));
	command_success_nodata(si, "--------------------------------");

	mowgli_patricia_foreach(global_template_dict, display_template, si);

	command_success_nodata(si, "--------------------------------");
	command_success_nodata(si, _("End of network wide template list."));
}

// TEMPLATE [channel] [template] [flags]
static void
cs_cmd_template(struct sourceinfo *si, int parc, char *parv[])
{
	struct metadata *md;
	bool operoverride = false, changechanacs = false;
	size_t l;
	char *channel = parv[0];
	char *target = parv[1];
	struct mychan *mc;
	unsigned int oldflags, newflags = 0, addflags, removeflags, restrictflags;
	char *p, *q, *r;
	char ss[40], newstr[400];
	bool found, denied;

	(void) memset(newstr, 0x00, sizeof newstr);

	if (!channel)
	{
		list_generic_flags(si);
		logcommand(si, CMDLOG_GET, "TEMPLATE");
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
		return;
	}

	if (!target)
	{
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

		if (metadata_find(mc, "private:close:closer") && !has_priv(si, PRIV_CHAN_AUSPEX))
		{
			command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, channel);
			return;
		}

		md = metadata_find(mc, "private:templates");

		if (md != NULL)
		{
			/* TRANSLATORS: Adjust these numbers only if the translated column
			 * headers would exceed that length. Pay particular attention to
			 * also changing the numbers in the format string inside the loop
			 * below to match them, and beware that these format strings are
			 * shared across multiple files!
			 */
			command_success_nodata(si, _("%-20s %s"), _("Name"), _("Flags"));
			command_success_nodata(si, "--------------------------------");

			p = md->value;
			while (p != NULL)
			{
				while (*p == ' ')
					p++;
				q = strchr(p, '=');
				if (q == NULL)
					break;
				r = strchr(q, ' ');
				command_success_nodata(si, _("%-20.*s %.*s"), (int)(q - p), p, r != NULL ? (int)(r - q - 1) : (int)strlen(q + 1), q + 1);
				p = r;
			}

			command_success_nodata(si, "--------------------------------");
			command_success_nodata(si, _("End of \2%s\2 TEMPLATE listing."), mc->name);
		}
		else
			command_success_nodata(si, _("No templates set on channel \2%s\2."), mc->name);

		if (operoverride)
			logcommand(si, CMDLOG_ADMIN, "TEMPLATE: \2%s\2 (oper override)", mc->name);
		else
			logcommand(si, CMDLOG_GET, "TEMPLATE: \2%s\2", mc->name);
	}
	else
	{
		char *flagstr = parv[2];

		if (!si->smu)
		{
			command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
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
				command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
				return;
			}
			restrictflags = allow_flags(mc, restrictflags);
		}

		if (metadata_find(mc, "private:close:closer"))
		{
			command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, channel);
			return;
		}

		if (!target || !flagstr)
		{
			command_fail(si, fault_needmoreparams, _("Usage: TEMPLATE %s [target flags]"), channel);
			return;
		}

		if (*target == '+' || *target == '-' || *target == '=')
		{
			command_fail(si, fault_badparams, _("Invalid template name \2%s\2."), target);
			return;
		}
		l = strlen(target);

		if (*flagstr == '!' && (flagstr[1] == '+' || flagstr[1] == '-' || flagstr[1] == '='))
		{
			changechanacs = true;
			flagstr++;
		}

		if (*flagstr == '+' || *flagstr == '-' || *flagstr == '=')
		{
			flags_make_bitmasks(flagstr, &addflags, &removeflags);
			if (addflags == 0 && removeflags == 0)
			{
				command_fail(si, fault_badparams, _("No valid flags given; use \2/msg %s HELP FLAGS\2 "
				                                    "for a list"), chansvs.me->disp);
				return;
			}
		}
		else
		{
			// allow copying templates as well
			addflags = get_template_flags(mc, flagstr);
			if (addflags == 0)
			{
				command_fail(si, fault_nosuch_key, _("Invalid template name given; use \2/msg %s "
				                                     "TEMPLATE %s\2 for a list"), chansvs.me->disp,
				                                     mc->name);
				return;
			}
			removeflags = ca_all & ~addflags;
		}

		// if adding +F, also add +f
		if (addflags & CA_FOUNDER)
		{
			addflags |= CA_FLAGS;
			removeflags &= ~CA_FLAGS;
		}
		// if removing +f, also remove +F
		else if (removeflags & CA_FLAGS)
		{
			removeflags |= CA_FOUNDER;
			addflags &= ~CA_FOUNDER;
		}

		found = denied = false;
		oldflags = 0;

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
				if ((size_t)(q - p) == l && !strncasecmp(target, p, l))
				{
					found = true;
					oldflags = flags_to_bitmask(ss, 0);
					oldflags &= ca_all;
					addflags &= ~oldflags;
					removeflags &= oldflags & ~addflags;

					// no change?
					if ((addflags | removeflags) == 0)
						break;

					/* attempting to add bad flag? */
					/* attempting to remove bad flag? */
					/* attempting to manipulate something with more privs? */
					if (~restrictflags & addflags ||
							~restrictflags & removeflags ||
							~restrictflags & oldflags)
					{
						denied = true;
						break;
					}
					newflags = (oldflags | addflags) & ~removeflags;
					if (newflags == 0)
					{
						if (p == md->value)
							/* removing first entry,
							 * zap the space after it */
							mowgli_strlcpy(newstr, r != NULL ? r + 1 : "", sizeof newstr);
						else
						{
							// otherwise, zap the space before it
							p--;
							mowgli_strlcpy(newstr + (p - md->value), r != NULL ? r : "", sizeof newstr - (p - md->value));
						}
					}
					else
						snprintf(newstr + (p - md->value), sizeof newstr - (p - md->value), "%s=%s%s", target, bitmask_to_flags(newflags), r != NULL ? r : "");
					break;
				}
				p = r;
			}
		}
		if (!found)
		{
			if (l == 3 && !chansvs.hide_xop && (!strcasecmp(target, "SOP") ||
						!strcasecmp(target, "AOP") ||
						!strcasecmp(target, "HOP") ||
						!strcasecmp(target, "VOP")))
			{
				oldflags = get_template_flags(NULL, target);
				addflags &= ~oldflags;
				removeflags &= oldflags & ~addflags;
				newflags = (oldflags | addflags) & ~removeflags;
				if (newflags == 0)
					removeflags = 0;
				if ((addflags | removeflags) != 0)
					command_success_nodata(si, _("Redefining built-in template \2%s\2."), target);
			}
			else
			{
				removeflags = 0;
				newflags = addflags;
			}
			if ((addflags | removeflags) == 0)
				;
			else if (~restrictflags & addflags ||
					~restrictflags & removeflags ||
					~restrictflags & oldflags)
				denied = true;
			else if (md != NULL)
				snprintf(newstr + strlen(newstr), sizeof newstr - strlen(newstr), " %s=%s", target, bitmask_to_flags(newflags));
			else
				snprintf(newstr, sizeof newstr, "%s=%s", target, bitmask_to_flags(newflags));
		}
		if (oldflags == 0 && has_ctrl_chars(target))
		{
			command_fail(si, fault_badparams, _("Invalid template name \2%s\2."), target);
			return;
		}
		if ((addflags | removeflags) == 0)
		{
			if (oldflags != 0)
				command_fail(si, fault_nochange, _("Template \2%s\2 on \2%s\2 unchanged."), target, channel);
			else
				command_fail(si, fault_nosuch_key, _("No such template \2%s\2 on \2%s\2."), target, channel);
			return;
		}
		if (denied)
		{
			command_fail(si, fault_noprivs, _("You are not allowed to set \2%s\2 on template \2%s\2 in \2%s\2."), bitmask_to_flags2(addflags, removeflags), target, mc->name);
			return;
		}
		if (strlen(newstr) >= 300)
		{
			command_fail(si, fault_toomany, _("Sorry, too many templates on \2%s\2."), channel);
			return;
		}
		p = md != NULL ? md->value : NULL;
		while (p != NULL)
		{
			while (*p == ' ')
			p++;
			q = strchr(p, '=');
			if (q == NULL)
					break;
			r = strchr(q, ' ');
			snprintf(ss,sizeof ss,"%.*s",r != NULL ? (int)(r - q - 1) : (int)strlen(q + 1), q + 1);
			if (flags_to_bitmask(ss,0) == newflags)
			{
				command_fail(si, fault_alreadyexists, _("The template \2%.*s\2 already has flags \2%.*s\2."), (int)(q - p), p, r != NULL ? (int)(r - q - 1) : (int)strlen(q + 1), q + 1);
				return;
			}

			p = r;
		}

		if (newstr[0] == '\0')
			metadata_delete(mc, "private:templates");
		else
			metadata_add(mc, "private:templates", newstr);
		if (oldflags == 0)
			command_success_nodata(si, _("Added template \2%s\2 with flags \2%s\2 in \2%s\2."), target, bitmask_to_flags(newflags), channel);
		else if (newflags == 0)
			command_success_nodata(si, _("Removed template \2%s\2 from \2%s\2."), target, channel);
		else
			command_success_nodata(si, _("Changed template \2%s\2 to \2%s\2 in \2%s\2."), target, bitmask_to_flags(newflags), channel);

		flagstr = bitmask_to_flags2(addflags, removeflags);
		if (changechanacs)
		{
			mowgli_node_t *n, *tn;
			struct chanacs *ca;
			unsigned int changes = 0, founderskipped = 0;
			char flagstr2[128];

			MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chanacs.head)
			{
				ca = n->data;
				if (ca->level != oldflags)
					continue;
				if ((addflags | removeflags) & CA_FOUNDER)
				{
					founderskipped++;
					continue;
				}
				changes++;
				chanacs_modify_simple(ca, newflags, ~newflags, si->smu);
				chanacs_close(ca);
			}
			logcommand(si, CMDLOG_SET, "TEMPLATE: \2%s\2 \2%s\2 !\2%s\2 (\2%u\2 changes)", mc->name, target, flagstr, changes);
			mowgli_strlcpy(flagstr2, flagstr, sizeof flagstr2);
			if (changes > 0)
				verbose(mc, "\2%s\2 set \2%s\2 on %u access entries with flags \2%s\2.", get_source_name(si), flagstr2, changes, bitmask_to_flags(oldflags));
			command_success_nodata(si, ngettext(N_("%u access entry updated accordingly."),
			                                    N_("%u access entries updated accordingly."),
			                                    changes), changes);
			if (founderskipped)
				command_success_nodata(si, _("Not updating \2%u\2 access entries involving founder status. Please do it manually."), founderskipped);
		}
		else
			logcommand(si, CMDLOG_SET, "TEMPLATE: \2%s\2 \2%s\2 \2%s\2", mc->name, target, flagstr);
	}
}

static struct command cs_flags = {
	.name           = "TEMPLATE",
	.desc           = N_("Manipulates predefined sets of flags."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_template,
	.help           = { .path = "cservice/template" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_flags);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_flags);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/template", MODULE_UNLOAD_CAPABILITY_OK)
