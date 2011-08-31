/*
 * Copyright (c) 2005-2006 Jilles Tjoelker, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService TEMPLATE functions.
 *
 */

#include "atheme.h"
#include "template.h"

DECLARE_MODULE_V1
(
	"chanserv/template", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void list_generic_flags(sourceinfo_t *si);

static void cs_cmd_template(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_flags = { "TEMPLATE", N_("Manipulates predefined sets of flags."),
                        AC_NONE, 3, cs_cmd_template, { .path = "cservice/template" } };

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_flags);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_flags);
}

static int display_template(const char *key, void *data, void *privdata)
{
	sourceinfo_t *si = privdata;
	default_template_t *def_t = data;
	unsigned int vopflags;

	vopflags = get_global_template_flags("VOP");
	if (def_t->flags == vopflags && !strcasecmp(key, "HOP"))
		return 0;

	command_success_nodata(si, "%-20s %s", key, bitmask_to_flags(def_t->flags));

	return 0;
}

static void list_generic_flags(sourceinfo_t *si)
{
	command_success_nodata(si, "%-20s %s", _("Name"), _("Flags"));
	command_success_nodata(si, "%-20s %s", "--------------------", "-----");

	mowgli_patricia_foreach(global_template_dict, display_template, si);

	command_success_nodata(si, "%-20s %s", "--------------------", "-----");
	command_success_nodata(si, _("End of network wide template list."));
}

/* TEMPLATE [channel] [template] [flags] */
static void cs_cmd_template(sourceinfo_t *si, int parc, char *parv[])
{
	metadata_t *md;
	bool operoverride = false, changechanacs = false;
	size_t l;
	char *channel = parv[0];
	char *target = parv[1];
	mychan_t *mc;
	unsigned int oldflags, newflags = 0, addflags, removeflags, restrictflags;
	char *p, *q, *r;
	char ss[40], newstr[400];
	bool found, denied;

	if (!channel)
	{
		list_generic_flags(si);
		logcommand(si, CMDLOG_GET, "TEMPLATE");
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!target)
	{
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
		
		if (metadata_find(mc, "private:close:closer") && !has_priv(si, PRIV_CHAN_AUSPEX))
		{
			command_fail(si, fault_noprivs, _("\2%s\2 is closed."), channel);
			return;
		}

		md = metadata_find(mc, "private:templates");

		if (md != NULL)
		{
			command_success_nodata(si, "%-20s %s", _("Name"), _("Flags"));
			command_success_nodata(si, "%-20s %s", "--------------------", "-----");

			p = md->value;
			while (p != NULL)
			{
				while (*p == ' ')
					p++;
				q = strchr(p, '=');
				if (q == NULL)
					break;
				r = strchr(q, ' ');
				command_success_nodata(si, "%-20.*s %.*s", (int)(q - p), p, r != NULL ? (int)(r - q - 1) : (int)strlen(q + 1), q + 1);
				p = r;
			}

			command_success_nodata(si, "%-20s %s", "--------------------", "-----");
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
			command_fail(si, fault_noprivs, _("You are not logged in."));
			return;
		}

		/* founder may always set flags -- jilles */
		restrictflags = chanacs_source_flags(mc, si);
		if (restrictflags & CA_FOUNDER)
			restrictflags = ca_all;
		else
		{
			if (!(restrictflags & CA_FLAGS))
			{
				command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
				return;
			}
			restrictflags = allow_flags(mc, restrictflags);
		}
		
		if (metadata_find(mc, "private:close:closer"))
		{
			command_fail(si, fault_noprivs, _("\2%s\2 is closed."), channel);
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
				command_fail(si, fault_badparams, _("No valid flags given, use /%s%s HELP FLAGS for a list"), ircd->uses_rcommand ? "" : "msg ", chansvs.me->disp);
				return;
			}
		}
		else
		{
			/* allow copying templates as well */
			addflags = get_template_flags(mc, flagstr);
			if (addflags == 0)
			{
				command_fail(si, fault_nosuch_key, _("Invalid template name given, use /%s%s TEMPLATE %s for a list"), ircd->uses_rcommand ? "" : "msg ", chansvs.me->disp, mc->name);
				return;
			}
			removeflags = ca_all & ~addflags;
		}

		/* if adding +F, also add +f */
		if (addflags & CA_FOUNDER)
			addflags |= CA_FLAGS, removeflags &= ~CA_FLAGS;
		/* if removing +f, also remove +F */
		else if (removeflags & CA_FLAGS)
			removeflags |= CA_FOUNDER, addflags &= ~CA_FOUNDER;

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
					/* no change? */
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
							/* otherwise, zap the space before it */
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
			chanacs_t *ca;
			int changes = 0, founderskipped = 0;
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
				chanacs_modify_simple(ca, newflags, ~newflags);
				chanacs_close(ca);
			}
			logcommand(si, CMDLOG_SET, "TEMPLATE: \2%s\2 \2%s\2 !\2%s\2 (\2%d\2 changes)", mc->name, target, flagstr, changes);
			mowgli_strlcpy(flagstr2, flagstr, sizeof flagstr2);
			if (changes > 0)
				verbose(mc, "\2%s\2 set \2%s\2 on %d access entries with flags \2%s\2.", get_source_name(si), flagstr2, changes, bitmask_to_flags(oldflags));
			command_success_nodata(si, _("%d access entries updated accordingly."), changes);
			if (founderskipped)
				command_success_nodata(si, _("Not updating %d access entries involving founder status. Please do it manually."), founderskipped);
		}
		else
			logcommand(si, CMDLOG_SET, "TEMPLATE: \2%s\2 \2%s\2 \2%s\2", mc->name, target, flagstr);
		/*verbose(mc, "Flags \2%s\2 were set on template \2%s\2 in \2%s\2.", flagstr, target, channel);*/
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
