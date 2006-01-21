/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService RECOVER functions.
 *
 * $Id: recover.c 4639 2006-01-21 22:06:41Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/recover", FALSE, _modinit, _moddeinit,
	"$Id: recover.c 4639 2006-01-21 22:06:41Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_recover(char *origin);

command_t cs_recover = { "RECOVER", "Regain control of your channel.",
                        AC_NONE, cs_cmd_recover };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");

        command_add(&cs_recover, cs_cmdtree);
	help_addentry(cs_helptree, "RECOVER", "help/cservice/recover", NULL);
}

void _moddeinit()
{
	command_delete(&cs_recover, cs_cmdtree);
	help_delentry(cs_helptree, "RECOVER");
}

static void cs_cmd_recover(char *origin)
{
	user_t *u = user_find_named(origin);
	chanuser_t *cu, *origin_cu = NULL;
	mychan_t *mc;
	node_t *n;
	char *name = strtok(NULL, " ");
	char hostbuf[BUFSIZE], hostbuf2[BUFSIZE];

	if (!name)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "RECOVER");
		notice(chansvs.nick, origin, "Syntax: RECOVER <#channel>");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", name);
		return;
	}

	if (!mc->chan)
	{
		notice(chansvs.nick, origin, "\2%s\2 does not exist.", name);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_RECOVER))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	verbose(mc, "\2%s\2 used RECOVER.", origin);
	logcommand(chansvs.me, u, CMDLOG_SET, "%s RECOVER", mc->name);

	/* deop everyone */
	LIST_FOREACH(n, mc->chan->members.head)
	{
		cu = (chanuser_t *)n->data;

		if (cu->user == u)
			origin_cu = cu;
		else if (is_internal_client(cu->user))
			;
		else
		{
			if ((CMODE_OP & cu->modes))
			{
				cmode(chansvs.nick, mc->chan->name, "-o", CLIENT_NAME(cu->user));
				cu->modes &= ~CMODE_OP;
			}
			if (ircd->uses_halfops && (ircd->halfops_mode & cu->modes))
			{
				char minush[3];
				strlcpy(minush, ircd->halfops_mchar, 3);
				minush[0] = '-';
				cmode(chansvs.nick, mc->chan->name, minush, CLIENT_NAME(cu->user));
				cu->modes &= ~ircd->halfops_mode;
			}
		}
	}

	/* remove modes that keep people out */
	if (CMODE_LIMIT & mc->chan->modes)
	{
		cmode(chansvs.nick, mc->chan->name, "-l", NULL);
		mc->chan->modes &= ~CMODE_LIMIT;
		mc->chan->limit = 0;
	}

	if (CMODE_KEY & mc->chan->modes)
	{
		cmode(chansvs.nick, mc->chan->name, "-k", mc->chan->key);
		mc->chan->modes &= ~CMODE_KEY;
		free(mc->chan->key);
		mc->chan->key = NULL;
	}

	if (origin_cu != NULL)
	{
		if (!(CMODE_OP & origin_cu->modes))
			cmode(chansvs.nick, mc->chan->name, "+o", CLIENT_NAME(u));
		origin_cu->modes |= CMODE_OP;
	}

	if (origin_cu != NULL || (chanacs_user_flags(mc, u) & (CA_OP | CA_AUTOOP)))
	{

		cmode(chansvs.nick, mc->chan->name, "+im", NULL);
		mc->chan->modes |= CMODE_INVITE | CMODE_MOD;
	}
	else if (CMODE_INVITE & mc->chan->modes)
	{
		cmode(chansvs.nick, mc->chan->name, "-i", NULL);
		mc->chan->modes &= ~CMODE_INVITE;
	}

	/* unban the user */
	snprintf(hostbuf, BUFSIZE, "%s!%s@%s", u->nick, u->user, u->host);
	snprintf(hostbuf2, BUFSIZE, "%s!%s@%s", u->nick, u->user, u->vhost);

	LIST_FOREACH(n, mc->chan->bans.head)
	{
		chanban_t *cb = n->data;

		if (cb->type != 'b')
			continue;
		if (!match(cb->mask, hostbuf) || !match(cb->mask, hostbuf2))
		{
			cmode(chansvs.nick, mc->chan->name, "-b", cb->mask);
			chanban_delete(cb);
		}
	}

	cmode(NULL); /* flush stacker */

	if (origin_cu == NULL)
	{
		/* set an exempt on the user calling this */
		/* no exception list structure right now */
		/* XXX check to see if the ircd even does +e */
		snprintf(hostbuf, BUFSIZE, "+e %s", hostbuf2);
		mode_sts(chansvs.nick, mc->chan->name, hostbuf);

		/* invite them back. */
		invite_sts(chansvs.me->me, u, mc->chan);
		notice(chansvs.nick, origin, "Recover complete for \2%s\2, ban exception \2%s\2 added.", mc->chan->name, hostbuf2);
	}
	else
		notice(chansvs.nick, origin, "Recover complete for \2%s\2.", mc->chan->name);
}
