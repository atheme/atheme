/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService XOP functions.
 *
 * $Id: xop.c 6577 2006-09-30 21:17:34Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/xop", FALSE, _modinit, _moddeinit,
	"$Id: xop.c 6577 2006-09-30 21:17:34Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

/* the individual command stuff, now that we've reworked, hardcode ;) --w00t */
static void cs_xop_do_list(mychan_t *mc, char *origin, uint32_t level, char *leveldesc, int operoverride);
static void cs_xop_do_add(mychan_t *mc, myuser_t *mu, char *origin, char *target, uint32_t level, char *leveldesc, uint32_t restrictflags);
static void cs_xop_do_del(mychan_t *mc, myuser_t *mu, char *origin, char *target, uint32_t level, char *leveldesc);

static void cs_cmd_sop(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_aop(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_hop(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_vop(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_forcexop(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_sop = { "SOP", "Manipulates a channel SOP list.",
                        AC_NONE, 3, cs_cmd_sop };
command_t cs_aop = { "AOP", "Manipulates a channel AOP list.",
                        AC_NONE, 3, cs_cmd_aop };
command_t cs_hop = { "HOP", "Manipulates a channel HOP list.",
			AC_NONE, 3, cs_cmd_hop };
command_t cs_vop = { "VOP", "Manipulates a channel VOP list.",
                        AC_NONE, 3, cs_cmd_vop };
command_t cs_forcexop = { "FORCEXOP", "Forces access levels to xOP levels.",
                         AC_NONE, 1, cs_cmd_forcexop };

list_t *cs_cmdtree, *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_aop, cs_cmdtree);
        command_add(&cs_sop, cs_cmdtree);
	command_add(&cs_hop, cs_cmdtree);
        command_add(&cs_vop, cs_cmdtree);
	command_add(&cs_forcexop, cs_cmdtree);

	help_addentry(cs_helptree, "SOP", "help/cservice/xop", NULL);
	help_addentry(cs_helptree, "AOP", "help/cservice/xop", NULL);
	help_addentry(cs_helptree, "VOP", "help/cservice/xop", NULL);
	help_addentry(cs_helptree, "HOP", "help/cservice/xop", NULL);
	help_addentry(cs_helptree, "FORCEXOP", "help/cservice/forcexop", NULL);
}

void _moddeinit()
{
	command_delete(&cs_aop, cs_cmdtree);
	command_delete(&cs_sop, cs_cmdtree);
	command_delete(&cs_hop, cs_cmdtree);
	command_delete(&cs_vop, cs_cmdtree);
	command_delete(&cs_forcexop, cs_cmdtree);

	help_delentry(cs_helptree, "SOP");
	help_delentry(cs_helptree, "AOP");
	help_delentry(cs_helptree, "VOP");
	help_delentry(cs_helptree, "HOP");
	help_delentry(cs_helptree, "FORCEXOP");
}

static void cs_xop(sourceinfo_t *si, int parc, char *parv[], uint32_t level, char *leveldesc)
{
	myuser_t *mu;
	mychan_t *mc;
	int operoverride = 0;
	uint32_t restrictflags;
	char *chan = parv[0];
	char *cmd = parv[1];
	char *uname = parv[2];

	if (!cmd || !chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "xOP");
		command_fail(si, fault_needmoreparams, "Syntax: SOP|AOP|HOP|VOP <#channel> ADD|DEL|LIST <nickname>");
		return;
	}

	if ((strcasecmp("LIST", cmd)) && (!uname))
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "xOP");
		command_fail(si, fault_needmoreparams, "Syntax: SOP|AOP|HOP|VOP <#channel> ADD|DEL|LIST <nickname>");
		return;
	}

	/* make sure they're registered, logged in
	 * and the founder of the channel before
	 * we go any further.
	 */
	if (!si->smu)
	{
		/* if they're opers and just want to LIST, they don't have to log in */
		if (!(has_priv(si->su, PRIV_CHAN_AUSPEX) && !strcasecmp("LIST", cmd)))
		{
			command_fail(si, fault_noprivs, "You are not logged in.");
			return;
		}
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "The channel \2%s\2 is not registered.", chan);
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer") && (!has_priv(si->su, PRIV_CHAN_AUSPEX) || strcasecmp("LIST", cmd)))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
		return;
	}

	/* ADD */
	if (!strcasecmp("ADD", cmd))
	{
		mu = myuser_find_ext(uname);

		/* As in /cs flags, allow founder to do anything */
		if (is_founder(mc, si->smu))
			restrictflags = CA_ALL;
		else
			restrictflags = chanacs_source_flags(mc, si);
		/* The following is a bit complicated, to allow for
		 * possible future denial of granting +f */
		if (!(restrictflags & CA_FLAGS))
		{
			command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
			return;
		}
		restrictflags = allow_flags(restrictflags);
		if ((restrictflags & level) != level)
		{
			command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
			return;
		}
		cs_xop_do_add(mc, mu, si->su->nick, uname, level, leveldesc, restrictflags);
	}

	else if (!strcasecmp("DEL", cmd))
	{
		mu = myuser_find_ext(uname);

		/* As in /cs flags, allow founder to do anything -- fix for #64: allow self removal. */
		if (is_founder(mc, si->smu) || mu == si->smu)
			restrictflags = CA_ALL;
		else
			restrictflags = chanacs_source_flags(mc, si);
		/* The following is a bit complicated, to allow for
		 * possible future denial of granting +f */
		if (!(restrictflags & CA_FLAGS))
		{
			command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
			return;
		}
		restrictflags = allow_flags(restrictflags);
		if ((restrictflags & level) != level)
		{
			command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
			return;
		}
		cs_xop_do_del(mc, mu, si->su->nick, uname, level, leveldesc);
	}

	else if (!strcasecmp("LIST", cmd))
	{
		if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
		{
			if (has_priv(si->su, PRIV_CHAN_AUSPEX))
				operoverride = 1;
			else
			{
				command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
				return;
			}
		}
		cs_xop_do_list(mc, si->su->nick, level, leveldesc, operoverride);
	}
}

static void cs_cmd_sop(sourceinfo_t *si, int parc, char *parv[])
{
	cs_xop(si, parc, parv, chansvs.ca_sop, "SOP");
}

static void cs_cmd_aop(sourceinfo_t *si, int parc, char *parv[])
{
	cs_xop(si, parc, parv, chansvs.ca_aop, "AOP");
}

static void cs_cmd_vop(sourceinfo_t *si, int parc, char *parv[])
{
	cs_xop(si, parc, parv, chansvs.ca_vop, "VOP");
}

static void cs_cmd_hop(sourceinfo_t *si, int parc, char *parv[])
{
	/* Don't reject the command. This helps the rare case where
	 * a network switches to a non-halfop ircd: users can still
	 * remove pre-transition HOP entries.
	 */
	if (!ircd->uses_halfops)
		notice(chansvs.nick, si->su->nick, "Warning: Your IRC server does not support halfops.");

	cs_xop(si, parc, parv, chansvs.ca_hop, "HOP");
}


static void cs_xop_do_add(mychan_t *mc, myuser_t *mu, char *origin, char *target, uint32_t level, char *leveldesc, uint32_t restrictflags)
{
	char hostbuf[BUFSIZE];
	chanuser_t *cu;
	chanacs_t *ca;
	node_t *n;

	if (!mu)
	{
		/* we might be adding a hostmask */
		if (!validhostmask(target))
		{
			notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", target);
			return;
		}

		target = collapse(target);
		ca = chanacs_find_host_literal(mc, target, CA_NONE);
		if (ca != NULL && ca->level == level)
		{
			notice(chansvs.nick, origin, "\2%s\2 is already on the %s list for \2%s\2", target, leveldesc, mc->name);
			return;
		}

		if (ca != NULL)
		{
			if (ca->level & ~restrictflags)
			{
				notice(chansvs.nick, origin, "You are not authorized to modify the access entry for \2%s\2 on \2%s\2.", target, mc->name);
				return;
			}
			/* they have access? change it! */
			logcommand_user(chansvs.me, user_find_named(origin), CMDLOG_SET, "%s %s ADD %s (changed access)", mc->name, leveldesc, target);
			notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed to \2%s\2.", target, mc->name, leveldesc);
			verbose(mc, "\2%s\2 changed \2%s\2's access to \2%s\2.", origin, target, leveldesc);
			ca->level = level;
		}
		else
		{
			logcommand_user(chansvs.me, user_find_named(origin), CMDLOG_SET, "%s %s ADD %s", mc->name, leveldesc, target);
			notice(chansvs.nick, origin, "\2%s\2 has been added to the %s list for \2%s\2.", target, leveldesc, mc->name);
			verbose(mc, "\2%s\2 added \2%s\2 to the %s list.", origin, target, leveldesc);
			chanacs_add_host(mc, target, level);
		}

		/* run through the channel's user list and do it */
		/* make sure the channel exists */
		if (mc->chan == NULL)
			return;
		LIST_FOREACH(n, mc->chan->members.head)
		{
			cu = (chanuser_t *)n->data;

			strlcpy(hostbuf, cu->user->nick, BUFSIZE);
			strlcat(hostbuf, "!", BUFSIZE);
			strlcat(hostbuf, cu->user->user, BUFSIZE);
			strlcat(hostbuf, "@", BUFSIZE);
			strlcat(hostbuf, cu->user->vhost, BUFSIZE);

			if (match(target, hostbuf))
				continue;

			if (level & CA_AUTOOP)
			{
				if (!(cu->modes & CMODE_OP))
				{
					modestack_mode_param(chansvs.nick, mc->name, MTYPE_ADD, 'o', CLIENT_NAME(cu->user));
					cu->modes |= CMODE_OP;
				}
			}
			else if (ircd->uses_halfops && level & CA_AUTOHALFOP)
			{
				if (!(cu->modes & (CMODE_OP | ircd->halfops_mode)))
				{
					modestack_mode_param(chansvs.nick, mc->name, MTYPE_ADD, ircd->halfops_mchar[1], CLIENT_NAME(cu->user));
					cu->modes |= ircd->halfops_mode;
				}
			}
			else if (level & (CA_AUTOVOICE | CA_AUTOHALFOP))
			{
				/* XXX HOP should have +V */
				if (!(cu->modes & (CMODE_OP | ircd->halfops_mode | CMODE_VOICE)))
				{
					modestack_mode_param(chansvs.nick, mc->name, MTYPE_ADD, 'v', CLIENT_NAME(cu->user));
					cu->modes |= CMODE_VOICE;
				}
			}
		}
		return;
	}

	if (mu == mc->founder)
	{
		notice(chansvs.nick, origin, "\2%s\2 is the founder for \2%s\2 and may not be added to the %s list.", mu->name, mc->name, leveldesc);
		return;
	}

	ca = chanacs_find(mc, mu, CA_NONE);
	if (ca != NULL && ca->level == level)
	{
		notice(chansvs.nick, origin, "\2%s\2 is already on the %s list for \2%s\2.", mu->name, leveldesc, mc->name);
		return;
	}

	/* NEVEROP logic moved here
	 * Allow changing access level, but not adding
	 * -- jilles */
	if (MU_NEVEROP & mu->flags && (ca == NULL || ca->level == CA_AKICK))
	{
		notice(chansvs.nick, origin, "\2%s\2 does not wish to be added to access lists (NEVEROP set).", mu->name);
		return;
	}
	/*
	 * this is a little more cryptic than it used to be, but much cleaner. Functionally should be
	 * the same, with the exception that if they had access before, now it doesn't tell what it got
	 * changed from (I considered the effort to put an extra lookup in not worth it. --w00t
	 */
	/* just assume there's just one entry for that user -- jilles */

	if (ca != NULL)
	{
		if (ca->level & ~restrictflags)
		{
			notice(chansvs.nick, origin, "You are not authorized to modify the access entry for \2%s\2 on \2%s\2.", mu->name, mc->name);
			return;
		}
		/* they have access? change it! */
		logcommand_user(chansvs.me, user_find_named(origin), CMDLOG_SET, "%s %s ADD %s (changed access)", mc->name, leveldesc, mu->name);
		notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed to \2%s\2.", mu->name, mc->name, leveldesc);
		verbose(mc, "\2%s\2 changed \2%s\2's access to \2%s\2.", origin, mu->name, leveldesc);
		ca->level = level;
	}
	else
	{
		/* they have no access, add */
		logcommand_user(chansvs.me, user_find_named(origin), CMDLOG_SET, "%s %s ADD %s", mc->name, leveldesc, mu->name);
		notice(chansvs.nick, origin, "\2%s\2 has been added to the %s list for \2%s\2.", mu->name, leveldesc, mc->name);
		verbose(mc, "\2%s\2 added \2%s\2 to the %s list.", origin, mu->name, leveldesc);
		chanacs_add(mc, mu, level);
	}
	/* run through the channel's user list and do it */
	/* make sure the channel exists */
	if (mc->chan == NULL)
		return;
	LIST_FOREACH(n, mc->chan->members.head)
	{
		cu = (chanuser_t *)n->data;

		if (cu->user->myuser != mu)
			continue;

		if (level & CA_AUTOOP)
		{
			if (!(cu->modes & CMODE_OP))
			{
				modestack_mode_param(chansvs.nick, mc->name, MTYPE_ADD, 'o', CLIENT_NAME(cu->user));
				cu->modes |= CMODE_OP;
			}
		}
		else if (ircd->uses_halfops && level & CA_AUTOHALFOP)
		{
			if (!(cu->modes & (CMODE_OP | ircd->halfops_mode)))
			{
				modestack_mode_param(chansvs.nick, mc->name, MTYPE_ADD, ircd->halfops_mchar[1], CLIENT_NAME(cu->user));
				cu->modes |= ircd->halfops_mode;
			}
		}
		else if (level & (CA_AUTOVOICE | CA_AUTOHALFOP))
		{
			/* XXX HOP should have +V */
			if (!(cu->modes & (CMODE_OP | ircd->halfops_mode | CMODE_VOICE)))
			{
				modestack_mode_param(chansvs.nick, mc->name, MTYPE_ADD, 'v', CLIENT_NAME(cu->user));
				cu->modes |= CMODE_VOICE;
			}
		}
	}
}

static void cs_xop_do_del(mychan_t *mc, myuser_t *mu, char *origin, char *target, uint32_t level, char *leveldesc)
{
	chanacs_t *ca;
	
	/* let's finally make this sane.. --w00t */
	if (!mu)
	{
		/* we might be deleting a hostmask */
		if (!validhostmask(target))
		{
			notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", target);
			return;
		}

		if (!chanacs_find_host_literal(mc, target, level))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on the %s list for \2%s\2.", target, leveldesc, mc->name);
			return;
		}

		chanacs_delete_host(mc, target, level);
		verbose(mc, "\2%s\2 removed \2%s\2 from the %s list.", origin, target, leveldesc);
		logcommand_user(chansvs.me, user_find_named(origin), CMDLOG_SET, "%s %s DEL %s", mc->name, leveldesc, target);
		notice(chansvs.nick, origin, "\2%s\2 has been removed from the %s list for \2%s\2.", target, leveldesc, mc->name);
		return;
	}

	if (!(ca = chanacs_find(mc, mu, level)) || ca->level != level)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on the %s list for \2%s\2.", mu->name, leveldesc, mc->name);
		return;
	}

	/* just in case... -- jilles */
	if (mu == mc->founder)
	{
		notice(chansvs.nick, origin, "\2%s\2 is the founder for \2%s\2 and may not be removed from the %s list.", mu->name, mc->name, leveldesc);
		return;
	}

	chanacs_delete(mc, mu, level);
	notice(chansvs.nick, origin, "\2%s\2 has been removed from the %s list for \2%s\2.", mu->name, leveldesc, mc->name);
	logcommand_user(chansvs.me, user_find_named(origin), CMDLOG_SET, "%s %s DEL %s", mc->name, leveldesc, mu->name);
	verbose(mc, "\2%s\2 removed \2%s\2 from the %s list.", origin, mu->name, leveldesc);
}


static void cs_xop_do_list(mychan_t *mc, char *origin, uint32_t level, char *leveldesc, int operoverride)
{
	chanacs_t *ca;
	uint8_t i = 0;
	node_t *n;

	notice(chansvs.nick, origin, "%s list for \2%s\2:", leveldesc ,mc->name);
	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = (chanacs_t *)n->data;
		/* founder is never on any xop list -- jilles */
		if (ca->myuser != mc->founder && ca->level == level)
		{
			if (!ca->myuser)
				notice(chansvs.nick, origin, "%d: \2%s\2", ++i, ca->host);
			else if (LIST_LENGTH(&ca->myuser->logins))
				notice(chansvs.nick, origin, "%d: \2%s\2 (logged in)", ++i, ca->myuser->name);
			else
				notice(chansvs.nick, origin, "%d: \2%s\2 (not logged in)", ++i, ca->myuser->name);
		}
	}
	/* XXX */
	notice(chansvs.nick, origin, "Total of \2%d\2 %s in %s list of \2%s\2.", i, (i == 1) ? "entry" : "entries", leveldesc, mc->name);
	if (operoverride)
		logcommand_user(chansvs.me, user_find_named(origin), CMDLOG_ADMIN, "%s %s LIST (oper override)", mc->name, leveldesc);
	else
		logcommand_user(chansvs.me, user_find_named(origin), CMDLOG_GET, "%s %s LIST", mc->name, leveldesc);
}

static void cs_cmd_forcexop(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	chanacs_t *ca;
	mychan_t *mc = mychan_find(chan);
	node_t *n;
	int changes;
	uint32_t newlevel;
	char *desc;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FORCEXOP");
		command_fail(si, fault_needmoreparams, "Syntax: FORCEXOP <#channel>");
		return;
	}

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
		return;
	}

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
		return;
	}

	if (!is_founder(mc, si->smu))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}

	changes = 0;
	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (ca->level & CA_AKICK)
			continue;
		if (ca->myuser && is_founder(mc, ca->myuser))
			newlevel = CA_INITIAL, desc = "Founder";
		else if (!(~ca->level & chansvs.ca_sop))
			newlevel = chansvs.ca_sop, desc = "SOP";
		else if (ca->level == chansvs.ca_aop)
			newlevel = chansvs.ca_aop, desc = "AOP";
		else if (ca->level == chansvs.ca_hop)
			newlevel = chansvs.ca_hop, desc = "HOP";
		else if (ca->level == chansvs.ca_vop)
			newlevel = chansvs.ca_vop, desc = "VOP";
		else if (ca->level & (CA_SET | CA_RECOVER | CA_FLAGS))
			newlevel = chansvs.ca_sop, desc = "SOP";
		else if (ca->level & (CA_OP | CA_AUTOOP | CA_REMOVE))
			newlevel = chansvs.ca_aop, desc = "AOP";
		else if (ca->level & (CA_HALFOP | CA_AUTOHALFOP | CA_TOPIC))
			newlevel = chansvs.ca_hop, desc = "HOP";
		else /*if (ca->level & CA_AUTOVOICE)*/
			newlevel = chansvs.ca_vop, desc = "VOP";
#if 0
		else
			newlevel = 0;
#endif
		if (newlevel == ca->level)
			continue;
		changes++;
		command_success_nodata(si, "%s: %s -> %s", ca->myuser ? ca->myuser->name : ca->host, bitmask_to_flags(ca->level, chanacs_flags), desc);
		ca->level = newlevel;
	}
	command_success_nodata(si, "FORCEXOP \2%s\2 done (\2%d\2 changes)", mc->name, changes);
	if (changes > 0)
		verbose(mc, "\2%s\2 reset access levels to xOP (\2%d\2 changes)", si->su->nick, changes);
	logcommand(si, CMDLOG_SET, "%s FORCEXOP (%d changes)", mc->name, changes);
}
