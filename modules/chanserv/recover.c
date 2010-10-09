/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService RECOVER functions.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/recover", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_recover(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_recover = { "RECOVER", N_("Regain control of your channel."),
                        AC_NONE, 1, cs_cmd_recover, { .path = "cservice/recover" } };

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_recover);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_recover);
}

static void cs_cmd_recover(sourceinfo_t *si, int parc, char *parv[])
{
	chanuser_t *cu, *origin_cu = NULL;
	mychan_t *mc;
	mowgli_node_t *n, *tn;
	chanban_t *cb;
	char *name = parv[0];
	char hostbuf2[BUFSIZE];
	char e;
	bool added_exempt = false;
	int i;
	char str[3];

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RECOVER");
		command_fail(si, fault_needmoreparams, _("Syntax: RECOVER <#channel>"));
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), name);
		return;
	}
	
	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), name);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_RECOVER))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if (!mc->chan)
	{
		if (chansvs.changets)
		{
			/* Otherwise, our modes are likely ignored. */
			join(mc->name, chansvs.nick);
			mc->flags |= MC_INHABIT;
		}
		if (!mc->chan)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is currently empty."), name);
			return;
		}
	}

	command_add_flood(si, FLOOD_HEAVY);
	verbose(mc, "\2%s\2 used RECOVER.", get_source_name(si));
	logcommand(si, CMDLOG_DO, "RECOVER: \2%s\2", mc->name);

	/* deop everyone */
	MOWGLI_ITER_FOREACH(n, mc->chan->members.head)
	{
		cu = (chanuser_t *)n->data;

		if (cu->user == si->su)
			origin_cu = cu;
		else if (is_internal_client(cu->user))
			;
		else
		{
			if (ircd->uses_owner && (ircd->owner_mode & cu->modes))
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->owner_mchar[1], CLIENT_NAME(cu->user));
				cu->modes &= ~ircd->owner_mode;
			}
			if (ircd->uses_protect && (ircd->protect_mode & cu->modes))
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->protect_mchar[1], CLIENT_NAME(cu->user));
				cu->modes &= ~ircd->protect_mode;
			}
			if ((CSTATUS_OP & cu->modes))
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, 'o', CLIENT_NAME(cu->user));
				cu->modes &= ~CSTATUS_OP;
			}
			if (ircd->uses_halfops && (ircd->halfops_mode & cu->modes))
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->halfops_mchar[1], CLIENT_NAME(cu->user));
				cu->modes &= ~ircd->halfops_mode;
			}
		}
	}

	if (origin_cu == NULL)
	{
		/* if requester is not on channel,
		 * remove modes that keep people out */
		if (CMODE_LIMIT & mc->chan->modes)
			channel_mode_va(si->service->me, mc->chan, 1, "-l");
		if (CMODE_KEY & mc->chan->modes)
			channel_mode_va(si->service->me, mc->chan, 2, "-k", "*");

		/* stuff like join throttling
		 * XXX only remove modes that could keep people out
		 * -- jilles */
		str[0] = '-';
		str[2] = '\0';
		for (i = 0; ignore_mode_list[i].mode != '\0'; i++)
		{
			str[1] = ignore_mode_list[i].mode;
			if (mc->chan->extmodes[i] != NULL)
				channel_mode_va(si->service->me, mc->chan, 1, str);
		}
	}
	else
	{
		if (!(CSTATUS_OP & origin_cu->modes))
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, 'o', CLIENT_NAME(si->su));
		origin_cu->modes |= CSTATUS_OP;
	}

	if (origin_cu != NULL || (si->su != NULL && chanacs_source_flags(mc, si) & (CA_OP | CA_AUTOOP)))
	{

		channel_mode_va(si->service->me, mc->chan, 1, "+im");
	}
	else if (CMODE_INVITE & mc->chan->modes)
	{
		channel_mode_va(si->service->me, mc->chan, 1, "-i");
	}

	if (si->su != NULL)
	{
		/* unban the user */
		snprintf(hostbuf2, BUFSIZE, "%s!%s@%s", si->su->nick, si->su->user, si->su->vhost);

		for (n = next_matching_ban(mc->chan, si->su, 'b', mc->chan->bans.head); n != NULL; n = next_matching_ban(mc->chan, si->su, 'b', tn))
		{
			tn = n->next;
			cb = n->data;

			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, cb->type, cb->mask);
			chanban_delete(cb);
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
					modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, e, hostbuf2);
					added_exempt = true;
				}
			}
		}
	}

	modestack_flush_channel(mc->chan);

	/* invite them back. must have sent +i before this */
	if (origin_cu == NULL && si->su != NULL)
		invite_sts(si->service->me, si->su, mc->chan);

	if (added_exempt)
		command_success_nodata(si, _("Recover complete for \2%s\2, ban exception \2%s\2 added."), mc->chan->name, hostbuf2);
	else
		command_success_nodata(si, _("Recover complete for \2%s\2."), mc->chan->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
