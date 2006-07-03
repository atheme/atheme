/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService RECOVER functions.
 *
 * $Id: recover.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/recover", FALSE, _modinit, _moddeinit,
	"$Id: recover.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_recover(char *origin);

command_t cs_recover = { "RECOVER", "Regain control of your channel.",
                        AC_NONE, cs_cmd_recover };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

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
	char e;
	boolean_t added_exempt = FALSE;
	int i;
	char str[3];

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
				modestack_mode_param(chansvs.nick, mc->chan->name, MTYPE_DEL, 'o', CLIENT_NAME(cu->user));
				cu->modes &= ~CMODE_OP;
			}
			if (ircd->uses_halfops && (ircd->halfops_mode & cu->modes))
			{
				modestack_mode_param(chansvs.nick, mc->chan->name, MTYPE_DEL, ircd->halfops_mchar[1], CLIENT_NAME(cu->user));
				cu->modes &= ~ircd->halfops_mode;
			}
		}
	}

	if (origin_cu == NULL)
	{
		/* if requester is not on channel,
		 * remove modes that keep people out */
		if (CMODE_LIMIT & mc->chan->modes)
			channel_mode_va(chansvs.me->me, mc->chan, 1, "-l");
		if (CMODE_KEY & mc->chan->modes)
			channel_mode_va(chansvs.me->me, mc->chan, 2, "-k", "*");

		/* stuff like join throttling
		 * XXX only remove modes that could keep people out
		 * -- jilles */
		str[0] = '-';
		str[2] = '\0';
		for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
		{
			str[1] = ignore_mode_list[i].mode;
			if (mc->chan->extmodes[i] != NULL)
				channel_mode_va(chansvs.me->me, mc->chan, 1, str);
		}
	}
	else
	{
		if (!(CMODE_OP & origin_cu->modes))
			modestack_mode_param(chansvs.nick, mc->chan->name, MTYPE_ADD, 'o', CLIENT_NAME(u));
		origin_cu->modes |= CMODE_OP;
	}

	if (origin_cu != NULL || (chanacs_user_flags(mc, u) & (CA_OP | CA_AUTOOP)))
	{

		channel_mode_va(chansvs.me->me, mc->chan, 1, "+im");
	}
	else if (CMODE_INVITE & mc->chan->modes)
	{
		channel_mode_va(chansvs.me->me, mc->chan, 1, "-i");
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
			modestack_mode_param(chansvs.nick, mc->chan->name, MTYPE_DEL, 'b', cb->mask);
			chanban_delete(cb);
		}
	}

	if (origin_cu == NULL)
	{
		/* set an exempt on the user calling this */
		e = ircd->except_mchar;
		if (e != '\0')
		{
			if (!chanban_find(mc->chan, hostbuf2, e))
			{
				chanban_add(mc->chan, hostbuf2, e);
				modestack_mode_param(chansvs.nick, mc->chan->name, MTYPE_ADD, e, hostbuf2);
				added_exempt = TRUE;
			}
		}
	}

	modestack_flush_channel(mc->chan->name);

	/* invite them back. must have sent +i before this */
	if (origin_cu == NULL)
		invite_sts(chansvs.me->me, u, mc->chan);

	if (added_exempt)
		notice(chansvs.nick, origin, "Recover complete for \2%s\2, ban exception \2%s\2 added.", mc->chan->name, hostbuf2);
	else
		notice(chansvs.nick, origin, "Recover complete for \2%s\2.", mc->chan->name);
}
