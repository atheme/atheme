/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SYNC functions.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/sync", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_sync(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_sync = { "SYNC", "Forces channel statuses to flags.",
                        AC_NONE, 1, cs_cmd_sync, { .path = "contrib/sync" } };

void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_sync);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_sync);
}

static void cs_cmd_sync(sourceinfo_t *si, int parc, char *parv[])
{
	chanuser_t *cu;
	mychan_t *mc;
	mowgli_node_t *n, *tn;
	char *name = parv[0];
	int fl;
	bool noop;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SYNC");
		command_fail(si, fault_needmoreparams, "Syntax: SYNC <#channel>");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", name);
		return;
	}
	
	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", name);
		return;
	}

	if (!mc->chan)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 does not exist.", name);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_RECOVER))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}

	verbose(mc, "\2%s\2 used SYNC.", get_source_name(si));
	logcommand(si, CMDLOG_SET, "SYNC: \2%s\2", mc->name);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chan->members.head)
	{
		cu = (chanuser_t *)n->data;

		if (is_internal_client(cu->user))
			continue;

		fl = chanacs_user_flags(mc, cu->user);
		noop = mc->flags & MC_NOOP || (cu->user->myuser != NULL &&
				cu->user->myuser->flags & MU_NOOP);

		if (fl & CA_AKICK && !(fl & CA_REMOVE))
		{
			if (mc->chan->nummembers <= (mc->flags & MC_GUARD ? 2 : 1))
			{
				mc->flags |= MC_INHABIT;
				if (!(mc->flags & MC_GUARD))
					join(mc->name, chansvs.nick);
			}
			/* XXX duplicate the whole thing in cs_join()? */
			kick(chansvs.me->me, mc->chan, cu->user, "User is banned from this channel");
			continue;
		}
		if (ircd->uses_owner)
		{
			if (fl & CA_USEOWNER)
			{
				if (!noop && fl & CA_AUTOOP && !(ircd->owner_mode & cu->modes))
				{
					modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, ircd->owner_mchar[1], CLIENT_NAME(cu->user));
					cu->modes |= ircd->owner_mode;
				}
			}
			else if (ircd->owner_mode & cu->modes)
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->owner_mchar[1], CLIENT_NAME(cu->user));
				cu->modes &= ~ircd->owner_mode;
			}
		}
		if (ircd->uses_protect)
		{
			if (fl & CA_USEPROTECT)
			{
				if (!noop && fl & CA_AUTOOP && !(ircd->protect_mode & cu->modes) && !(ircd->uses_owner && cu->modes & ircd->owner_mode))
				{
					modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, ircd->protect_mchar[1], CLIENT_NAME(cu->user));
					cu->modes |= ircd->protect_mode;
				}
			}
			else if (ircd->protect_mode & cu->modes)
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->protect_mchar[1], CLIENT_NAME(cu->user));
				cu->modes &= ~ircd->protect_mode;
			}
		}
		if (fl & (CA_AUTOOP | CA_OP))
		{
			if (!noop && fl & CA_AUTOOP && !(CSTATUS_OP & cu->modes))
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, 'o', CLIENT_NAME(cu->user));
				cu->modes |= CSTATUS_OP;
			}
			continue;
		}
		if ((CSTATUS_OP & cu->modes))
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, 'o', CLIENT_NAME(cu->user));
			cu->modes &= ~CSTATUS_OP;
		}
		if (ircd->uses_halfops)
		{
			if (fl & (CA_AUTOHALFOP | CA_HALFOP))
			{
				if (!noop && fl & CA_AUTOHALFOP && !(ircd->halfops_mode & cu->modes))
				{
					modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, ircd->halfops_mchar[1], CLIENT_NAME(cu->user));
					cu->modes |= ircd->halfops_mode;
				}
				continue;
			}
			if (ircd->halfops_mode & cu->modes)
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->halfops_mchar[1], CLIENT_NAME(cu->user));
				cu->modes &= ~ircd->halfops_mode;
			}
		}
		if (fl & (CA_AUTOVOICE | CA_VOICE))
		{
			if (!noop && fl & CA_AUTOVOICE && !(CSTATUS_VOICE & cu->modes))
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, 'v', CLIENT_NAME(cu->user));
				cu->modes |= CSTATUS_VOICE;
			}
			continue;
		}
		if ((CSTATUS_VOICE & cu->modes))
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, 'v', CLIENT_NAME(cu->user));
			cu->modes &= ~CSTATUS_VOICE;
		}
	}

	command_success_nodata(si, "Sync complete for \2%s\2.", mc->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
