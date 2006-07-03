/*
 * Copyright (c) 2005 Jilles Tjoelker, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService TEMPLATE functions.
 *
 * $Id: template.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"
#include "template.h"

DECLARE_MODULE_V1
(
	"chanserv/template", FALSE, _modinit, _moddeinit,
	"$Id: template.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void list_generic_flags(char *origin);

static void cs_cmd_template(char *origin);

command_t cs_flags = { "TEMPLATE", "Manipulates predefined sets of flags.",
                        AC_NONE, cs_cmd_template };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_flags, cs_cmdtree);
	help_addentry(cs_helptree, "TEMPLATE", "help/cservice/template", NULL);
}

void _moddeinit()
{
	command_delete(&cs_flags, cs_cmdtree);
	help_delentry(cs_helptree, "TEMPLATE");
}

static void list_generic_flags(char *origin)
{
	notice(chansvs.nick, origin, "%-20s %s", "Name", "Flags");
	notice(chansvs.nick, origin, "%-20s %s", "--------------------", "-----");
	notice(chansvs.nick, origin, "%-20s %s", "SOP", bitmask_to_flags(chansvs.ca_sop, chanacs_flags));
	notice(chansvs.nick, origin, "%-20s %s", "AOP", bitmask_to_flags(chansvs.ca_aop, chanacs_flags));
	notice(chansvs.nick, origin, "%-20s %s", "HOP", bitmask_to_flags(chansvs.ca_hop, chanacs_flags));
	notice(chansvs.nick, origin, "%-20s %s", "VOP", bitmask_to_flags(chansvs.ca_vop, chanacs_flags));
	notice(chansvs.nick, origin, "%-20s %s", "--------------------", "-----");
	notice(chansvs.nick, origin, "End of network wide template list.");
}

/* TEMPLATE [channel] [template] [flags] */
static void cs_cmd_template(char *origin)
{
	user_t *u = user_find_named(origin);
	metadata_t *md;
	int operoverride = 0, l;
	char *channel = strtok(NULL, " ");
	char *target = strtok(NULL, " ");
	mychan_t *mc = mychan_find(channel);
	uint32_t oldflags, newflags = 0, addflags, removeflags, restrictflags;
	char *p, *q, *r;
	char ss[40], newstr[400];
	boolean_t found, denied;

	if (!channel)
	{
		list_generic_flags(origin);
		logcommand(chansvs.me, u, CMDLOG_GET, "TEMPLATE");
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", channel);
		return;
	}

	if (!target)
	{
		uint8_t i = 1;

		if (!chanacs_user_has_flag(mc, u, CA_ACLVIEW))
		{
			if (has_priv(u, PRIV_CHAN_AUSPEX))
				operoverride = 1;
			else
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
				return;
			}
		}
		
		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer") && !has_priv(u, PRIV_CHAN_AUSPEX))
		{
			notice(chansvs.nick, origin, "\2%s\2 is closed.", channel);
			return;
		}

		md = metadata_find(mc, METADATA_CHANNEL, "private:templates");

		if (md != NULL)
		{
			notice(chansvs.nick, origin, "%-20s %s", "Name", "Flags");
			notice(chansvs.nick, origin, "%-20s %s", "--------------------", "-----");

			p = md->value;
			while (p != NULL)
			{
				while (*p == ' ')
					p++;
				q = strchr(p, '=');
				if (q == NULL)
					break;
				r = strchr(q, ' ');
				notice(chansvs.nick, origin, "%-20.*s %.*s", (int)(q - p), p, r != NULL ? (int)(r - q - 1) : (int)strlen(q + 1), q + 1);
				i++;
				p = r;
			}

			notice(chansvs.nick, origin, "%-20s %s", "--------------------", "-----");
			notice(chansvs.nick, origin, "End of \2%s\2 TEMPLATE listing.", mc->name);
		}
		else
			notice(chansvs.nick, origin, "No templates set on channel \2%s\2.", mc->name);
		if (operoverride)
			logcommand(chansvs.me, u, CMDLOG_ADMIN, "%s TEMPLATE (oper override)", mc->name);
		else
			logcommand(chansvs.me, u, CMDLOG_GET, "%s TEMPLATE", mc->name);
	}
	else
	{
		char *flagstr = strtok(NULL, " ");

		if (!u->myuser)
		{
			notice(chansvs.nick, origin, "You are not logged in.");
			return;
		}

		/* probably no need to special-case founder here -- jilles */
#if 0
		if (is_founder(mc, u->myuser))
			restrictflags = CA_ALL;
		else
#endif
		{
			restrictflags = chanacs_user_flags(mc, u);
			if (!(restrictflags & CA_FLAGS))
			{
				notice(chansvs.nick, origin, "You are not authorized to execute this command.");
				return;
			}
			restrictflags = allow_flags(restrictflags);
		}
		
		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
		{
			notice(chansvs.nick, origin, "\2%s\2 is closed.", channel);
			return;
		}

		if (!target || !flagstr)
		{
			notice(chansvs.nick, origin, "Usage: TEMPLATE %s [target flags]", channel);
			return;
		}

		if (*target == '+' || *target == '-' || *target == '=')
		{
			notice(chansvs.nick, origin, "Invalid template name \2%s\2.", target);
			return;
		}
		l = strlen(target);
		/* don't allow redefining xop
		 * redefinition of future per-network other templates
		 * could be ok though */
		if (l == 3 && (!strcasecmp(target, "SOP") ||
					!strcasecmp(target, "AOP") ||
					!strcasecmp(target, "HOP") ||
					!strcasecmp(target, "VOP")))
		{
			notice(chansvs.nick, origin, "Cannot redefine built-in template \2%s\2.", target);
			return;
		}

		if (*flagstr == '+' || *flagstr == '-' || *flagstr == '=')
		{
			flags_make_bitmasks(flagstr, chanacs_flags, &addflags, &removeflags);
			if (addflags == 0 && removeflags == 0)
			{
				notice(chansvs.nick, origin, "No valid flags given, use /%s%s HELP FLAGS for a list", ircd->uses_rcommand ? "" : "msg ", chansvs.disp);
				return;
			}
		}
		else
		{
			/* allow copying templates as well */
			addflags = get_template_flags(mc, flagstr);
			if (addflags == 0)
			{
				notice(chansvs.nick, origin, "Invalid template name given, use /%s%s TEMPLATE %s for a list", ircd->uses_rcommand ? "" : "msg ", chansvs.disp, mc->name);
				return;
			}
			removeflags = CA_ALL & ~addflags;
		}

		found = denied = FALSE;
		oldflags = 0;

		md = metadata_find(mc, METADATA_CHANNEL, "private:templates");
		if (md != NULL)
		{
			p = md->value;
			strlcpy(newstr, p, sizeof newstr);
			while (p != NULL)
			{
				while (*p == ' ')
					p++;
				q = strchr(p, '=');
				if (q == NULL)
					break;
				r = strchr(q, ' ');
				strlcpy(ss, q, sizeof ss);
				if (r != NULL && r - q < (int)(sizeof ss - 1))
				{
					ss[r - q] = '\0';
				}
				if (q - p == l && !strncasecmp(target, p, q - p))
				{
					found = TRUE;
					oldflags = flags_to_bitmask(ss, chanacs_flags, 0);
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
						denied = TRUE;
						break;
					}
					newflags = (oldflags | addflags) & ~removeflags;
					if (newflags == 0)
					{
						if (p == md->value)
							/* removing first entry,
							 * zap the space after it */
							strlcpy(newstr, r != NULL ? r + 1 : "", sizeof newstr);
						else
						{
							/* otherwise, zap the space before it */
							p--;
							strlcpy(newstr + (p - md->value), r != NULL ? r : "", sizeof newstr - (p - md->value));
						}
					}
					else
						snprintf(newstr + (p - md->value), sizeof newstr - (p - md->value), "%s=%s%s", target, bitmask_to_flags(newflags, chanacs_flags), r != NULL ? r : "");
					break;
				}
				p = r;
			}
		}
		if (!found)
		{
			removeflags = 0;
			newflags = addflags;
			if ((addflags | removeflags) == 0)
				;
			else if (~restrictflags & addflags)
				denied = TRUE;
			else if (md != NULL)
				snprintf(newstr + strlen(newstr), sizeof newstr - strlen(newstr), " %s=%s", target, bitmask_to_flags(addflags, chanacs_flags));
			else
				snprintf(newstr, sizeof newstr, "%s=%s", target, bitmask_to_flags(newflags, chanacs_flags));
		}
		if ((addflags | removeflags) == 0)
		{
			if (oldflags != 0)
				notice(chansvs.nick, origin, "Template \2%s\2 on \2%s\2 unchanged.", target, channel);
			else
				notice(chansvs.nick, origin, "No such template \2%s\2 on \2%s\2.", target, channel);
			return;
		}
		if (denied)
		{
			notice(chansvs.nick, origin, "You are not allowed to set \2%s\2 on template \2%s\2 in \2%s\2.", bitmask_to_flags2(addflags, removeflags, chanacs_flags), target, mc->name);
			return;
		}
		if (strlen(newstr) >= 300)
		{
			notice(chansvs.nick, origin, "Sorry, too many templates on \2%s\2.", channel);
			return;
		}
		if (newstr[0] == '\0')
			metadata_delete(mc, METADATA_CHANNEL, "private:templates");
		else
			metadata_add(mc, METADATA_CHANNEL, "private:templates", newstr);
		if (oldflags == 0)
			notice(chansvs.nick, origin, "Added template \2%s\2 with flags \2%s\2 in \2%s\2.", target, bitmask_to_flags(newflags, chanacs_flags), channel);
		else if (newflags == 0)
			notice(chansvs.nick, origin, "Removed template \2%s\2 from \2%s\2.", target, channel);
		else
			notice(chansvs.nick, origin, "Changed template \2%s\2 to \2%s\2 in \2%s\2.", target, bitmask_to_flags(newflags, chanacs_flags), channel);
		flagstr = bitmask_to_flags2(addflags, removeflags, chanacs_flags);
		logcommand(chansvs.me, u, CMDLOG_SET, "%s TEMPLATE %s %s", mc->name, target, flagstr);
		/*verbose(mc, "Flags \2%s\2 were set on template \2%s\2 in \2%s\2.", flagstr, target, channel);*/
	}
}
