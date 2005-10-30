/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService XOP functions.
 *
 * $Id: xop.c 3273 2005-10-30 05:25:32Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/xop", FALSE, _modinit, _moddeinit,
	"$Id: xop.c 3273 2005-10-30 05:25:32Z alambert $",
	"Atheme Development Group <http://www.atheme.org>"
);

/* the individual command stuff, now that we've reworked, hardcode ;) --w00t */
static void cs_xop_do_list(mychan_t *mc, char *origin, uint32_t level);
static void cs_xop_do_add(mychan_t *mc, myuser_t *mu, char *origin, char *target, uint32_t level);
static void cs_xop_do_del(mychan_t *mc, myuser_t *mu, char *origin, char *target, uint32_t level);

static void cs_cmd_sop(char *origin);
static void cs_cmd_aop(char *origin);
static void cs_cmd_hop(char *origin);
static void cs_cmd_vop(char *origin);

command_t cs_sop = { "SOP", "Manipulates a channel SOP list.",
                        AC_NONE, cs_cmd_sop };
command_t cs_aop = { "AOP", "Manipulates a channel AOP list.",
                        AC_NONE, cs_cmd_aop };
command_t cs_hop = { "HOP", "Manipulates a channel HOP list.",
			AC_NONE, cs_cmd_hop };
command_t cs_vop = { "VOP", "Manipulates a channel VOP list.",
                        AC_NONE, cs_cmd_vop };

list_t *cs_cmdtree, *cs_helptree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");

        command_add(&cs_aop, cs_cmdtree);
        command_add(&cs_sop, cs_cmdtree);
	command_add(&cs_hop, cs_cmdtree);
        command_add(&cs_vop, cs_cmdtree);

	help_addentry(cs_helptree, "SOP", "help/cservice/xop", NULL);
	help_addentry(cs_helptree, "AOP", "help/cservice/xop", NULL);
	help_addentry(cs_helptree, "VOP", "help/cservice/xop", NULL);
	help_addentry(cs_helptree, "HOP", "help/cservice/xop", NULL);
}

void _moddeinit()
{
	command_delete(&cs_aop, cs_cmdtree);
	command_delete(&cs_sop, cs_cmdtree);
	command_delete(&cs_hop, cs_cmdtree);
	command_delete(&cs_vop, cs_cmdtree);

	help_delentry(cs_helptree, "SOP");
	help_delentry(cs_helptree, "AOP");
	help_delentry(cs_helptree, "VOP");
	help_delentry(cs_helptree, "HOP");
}

static void cs_xop(char *origin, uint32_t level)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	chanuser_t *cu;
	node_t *n;
	char *chan = strtok(NULL, " ");
	char *cmd = strtok(NULL, " ");
	char *uname = strtok(NULL, " ");

	if (!cmd || !chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2xOP\2.");
		notice(chansvs.nick, origin, "Syntax: SOP|AOP|HOP|VOP <#channel> ADD|DEL|LIST <nickname>");
		return;
	}

	if ((strcasecmp("LIST", cmd)) && (!uname))
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2xOP\2.");
		notice(chansvs.nick, origin, "Syntax: SOP|AOP|HOP|VOP <#channel> ADD|DEL|LIST <nickname>");
		return;
	}

	/* make sure they're registered, logged in
	 * and the founder of the channel before
	 * we go any further.
	 */
	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "The channel \2%s\2 is not registered.", chan);
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	mu = myuser_find(uname);

	/* ADD */
	if (!strcasecmp("ADD", cmd))
	{
		if (mu)
		{
			/* NEVEROP logic simplification and fix. Yes, this looks a little weird. --w00t */
			if (MU_NEVEROP & mu->flags)
			{
				notice(chansvs.nick, origin, "\2%s\2 does not wish to be added to access lists.", mu->name);
				return;
			}
		}
		if (!chanacs_user_has_flag(mc, u, CA_FLAGS))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}
		cs_xop_do_add(mc, mu, origin, uname, level);
	}

	else if (!strcasecmp("DEL", cmd))
	{
		if (!chanacs_user_has_flag(mc, u, CA_FLAGS))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}
		cs_xop_do_del(mc, mu, origin, uname, level);
	}

	else if (!strcasecmp("LIST", cmd))
	{
		if (!chanacs_user_has_flag(mc, u, CA_ACLVIEW))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}
		cs_xop_do_list(mc, origin, level);
	}
}

static void cs_cmd_sop(char *origin)
{
	cs_xop(origin, CA_SOP);
}

static void cs_cmd_aop(char *origin)
{
	cs_xop(origin, CA_AOP);
}

static void cs_cmd_vop(char *origin)
{
	cs_xop(origin, CA_VOP);
}

static void cs_cmd_hop(char *origin)
{
	/* Don't reject the command. This helps the rare case where
	 * a network switches to a non-halfop ircd: users can still
	 * remove pre-transition HOP entries.
	 */
	if (!ircd->uses_halfops)
		notice(chansvs.nick, origin, "Warning: Your IRC server does not support halfops.");

	cs_xop(origin, CA_HOP);
}


static void cs_xop_do_add(mychan_t *mc, myuser_t *mu, char *origin, char *target, uint32_t level)
{
	uint32_t modetoset = 0;
	char *leveldesc = NULL;
	char hostbuf[BUFSIZE];
	metadata_t *md;
	chanuser_t *cu;
	chanacs_t *ca;
	node_t *n;

	switch (level)
	{
		case CA_VOP:
			modetoset = CMODE_VOICE;
			leveldesc = "VOP";
			break;
		case CA_HOP:
			modetoset = ircd->halfops_mode;
			leveldesc = "HOP";
			break;
		case CA_AOP:
			modetoset = CMODE_OP;
			leveldesc = "AOP";
			break;
		case CA_SOP:		
			modetoset = ircd->protect_mode;
			leveldesc = "SOP";
			break;
	}

	if (!mu)
	{
		/* we might be adding a hostmask */
		if (!validhostmask(target))
		{
			notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", target);
			return;
		}

		if (chanacs_find_host_literal(mc, target, level))
		{
			notice(chansvs.nick, origin, "\2%s\2 is already on the %s list for \2%s\2", target, leveldesc, mc->name);
			return;
		}

		target = collapse(target);
		chanacs_add_host(mc, target, level);
		verbose(mc, "\2%s\2 added \2%s\2 to the %s list.", origin, target, leveldesc);
		notice(chansvs.nick, origin, "\2%s\2 has been added to the %s list for \2%s\2.", target, leveldesc, mc->name);

		/* run through the channel's user list and do it */
		LIST_FOREACH(n, mc->chan->members.head)
		{
			cu = (chanuser_t *)n->data;

			if (cu->modes & modetoset)
				return;

			hostbuf[0] = '\0';
			strlcat(hostbuf, cu->user->nick, BUFSIZE);
			strlcat(hostbuf, "!", BUFSIZE);
			strlcat(hostbuf, cu->user->user, BUFSIZE);
			strlcat(hostbuf, "@", BUFSIZE);
			strlcat(hostbuf, cu->user->host, BUFSIZE);

			/* this is ugly as sin. see bug #22 */
			switch (level)
			{
				case CA_VOP:
					if (should_voice_host(mc, hostbuf))
					{
						cmode(chansvs.nick, mc->name, "+v", cu->user->nick);
						cu->modes |= CMODE_VOICE;
					}
					break;
				case CA_HOP:
					if (should_halfop_host(mc, hostbuf))
					{
						cmode(chansvs.nick, mc->name, "+h", cu->user->nick);
						cu->modes |= ircd->halfops_mode;
					}
					break;
				case CA_AOP:
					/* fallthrough, temporary till #22 is fixed */
				case CA_SOP:
					if (should_op_host(mc, hostbuf))
					{
						cmode(chansvs.nick, mc->name, "+o", cu->user->nick);
						cu->modes |= CMODE_OP;
					}
			}
		}
		return;
	}

	if ((mu->flags & MU_ALIAS) && (md = metadata_find(mu, METADATA_USER, "private:alias:parent")))
	{
		/* This shouldn't ever happen, but just in case it does... */
		if (!(mu = myuser_find(md->value)))
			return;

		notice(chansvs.nick, origin, "\2%s\2 is an alias for \2%s\2. Adding entry under \2%s\2.", target, md->value, mu->name);
	}

	if (ca = chanacs_find(mc, mu, level))
	{
		notice(chansvs.nick, origin, "\2%s\2 is already on the %s list for \2%s\2.", mu->name, leveldesc, mc->name);
		return;
	}


	/*
	 * this is a little more cryptic than it used to be, but much cleaner. Functionally should be
	 * the same, with the exception that if they had access before, now it doesn't tell what it got
	 * changed from (I considered the effort to put an extra lookup in not worth it. --w00t
	 */

	if (ca = chanacs_find(mc, mu, CA_NONE))
	{
		/* they have access? remove it! */
		LIST_FOREACH(n, mc->chanacs.head)
		{
			/* seek! */
			ca = (chanacs_t *)n->data;
			if (ca->myuser->name == mu->name)
			{
				/* found an entry? kill it! */
				chanacs_delete(mc, mu, ca->level);
			}
                }
		notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed to \2%s\2.", mu->name, mc->name, leveldesc);
		verbose(mc, "\2%s\2 changed \2%s\2's access to \2%s\2.", origin, mu->name, leveldesc);
	}
	else
	{
		/* they have no access, add */
		notice(chansvs.nick, origin, "\2%s\2 has been added to the %s list for \2%s\2.", mu->name, leveldesc, mc->name);
		verbose(mc, "\2%s\2 added \2%s\2 to the %s list.", origin, mu->name, leveldesc);
	}

	/* add here to save a line */
	chanacs_add(mc, mu, level);
}

static void cs_xop_do_del(mychan_t *mc, myuser_t *mu, char *origin, char *target, uint32_t level)
{
	chanacs_t *ca;
	metadata_t *md;

	if (mu && (mu->flags & MU_ALIAS) && (md = metadata_find(mu, METADATA_USER, "private:alias:parent")))
	{
		/* This shouldn't ever happen, but just in case it does... */
		if (!(mu = myuser_find(md->value)))
			return;

		notice(chansvs.nick, origin, "\2%s\2 is an alias for \2%s\2. Deleting entry under \2%s\2.", target, md->value, mu->name);
	}

	/* VOP */
	if (level == CA_VOP)
	{
		if (!mu)
		{
			/* we might be deleting a hostmask */
			if (!validhostmask(target))
			{
				notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", target);
				return;
			}

			if (!chanacs_find_host_literal(mc, target, CA_VOP))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not on the VOP list for \2%s\2.", target, mc->name);
				return;
			}

			chanacs_delete_host(mc, target, CA_VOP);
			verbose(mc, "\2%s\2 removed \2%s\2 from the VOP list.", origin, target);
			notice(chansvs.nick, origin, "\2%s\2 has been removed from the VOP list for \2%s\2.", target, mc->name);
			return;
		}

		if (!(ca = chanacs_find(mc, mu, CA_VOP)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on the VOP list for \2%s\2.", mu->name, mc->name);
			return;
		}

		chanacs_delete(mc, mu, CA_VOP);
		notice(chansvs.nick, origin, "\2%s\2 has been removed from the VOP list for \2%s\2.", mu->name, mc->name);
		verbose(mc, "\2%s\2 removed \2%s\2 from the VOP list.", origin, mu->name);
		return;
	}

	/* HOP */
	if (level == CA_HOP)
	{
		if (!mu)
		{
			/* we might be deleting a hostmask */
			if (!validhostmask(target))
			{
				notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", target);
				return;
			}

			if (!chanacs_find_host_literal(mc, target, CA_HOP))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not on the HOP list for \2%s\2.", target, mc->name);
				return;
			}

			chanacs_delete_host(mc, target, CA_HOP);
			verbose(mc, "\2%s\2 removed \2%s\2 from the HOP list.", origin, target);
			notice(chansvs.nick, origin, "\2%s\2 has been removed from the HOP list for \2%s\2.", target, mc->name);
			return;
		}

		if (!(ca = chanacs_find(mc, mu, CA_HOP)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on the HOP list for \2%s\2.", mu->name, mc->name);
			return;
		}

		chanacs_delete(mc, mu, CA_HOP);
		notice(chansvs.nick, origin, "\2%s\2 has been removed from the HOP list for \2%s\2.", mu->name, mc->name);
		verbose(mc, "\2%s\2 removed \2%s\2 from the HOP list.", origin, mu->name);
		return;
	}

	/* AOP */
	if (level == CA_AOP)
	{
		if (!mu)
		{
			/* we might be deleting a hostmask */
			if (!validhostmask(target))
			{
				notice(chansvs.nick, origin, "\2%s\2 is neither a nickname nor a hostmask.", target);
				return;
			}

			if (!chanacs_find_host_literal(mc, target, CA_AOP))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not on the AOP list for \2%s\2.", target, mc->name);
				return;
			}

			chanacs_delete_host(mc, target, CA_AOP);
			verbose(mc, "\2%s\2 removed \2%s\2 from the AOP list.", origin, target);
			notice(chansvs.nick, origin, "\2%s\2 has been removed from the AOP list for \2%s\2.", target, mc->name);
			return;
		}

		if (!(ca = chanacs_find(mc, mu, CA_AOP)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on the AOP list for \2%s\2.", mu->name, mc->name);
			return;
		}

		chanacs_delete(mc, mu, CA_AOP);
		notice(chansvs.nick, origin, "\2%s\2 has been removed from the AOP list for \2%s\2.", mu->name, mc->name);
		verbose(mc, "\2%s\2 removed \2%s\2 from the AOP list.", origin, mu->name);
		return;
	}

	/* SOP */
	if (level == CA_SOP)
	{
		if (!mu)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", target);
			return;
		}

		if (!(ca = chanacs_find(mc, mu, CA_SOP)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on the SOP list for \2%s\2.", mu->name, mc->name);
			return;
		}

		chanacs_delete(mc, mu, CA_SOP);
		notice(chansvs.nick, origin, "\2%s\2 has been removed from the SOP list for \2%s\2.", mu->name, mc->name);
		verbose(mc, "\2%s\2 removed \2%s\2 from the SOP list.", origin, mu->name);
		return;
	}
}


static void cs_xop_do_list(mychan_t *mc, char *origin, uint32_t level)
{
	chanacs_t *ca;
	uint8_t i = 0;
	node_t *n;
	char *leveldesc = NULL;

	switch (level)
	{
		case CA_VOP:
			leveldesc = "VOP";
			break;
		case CA_HOP:
			leveldesc = "HOP";
			break;
		case CA_AOP:
			leveldesc = "AOP";
			break;
		case CA_SOP:
			leveldesc = "SOP";
			break;
	}


	notice(chansvs.nick, origin, "%s list for \2%s\2:", leveldesc ,mc->name);
	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = (chanacs_t *)n->data;
		if (ca->level == level)
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
}
