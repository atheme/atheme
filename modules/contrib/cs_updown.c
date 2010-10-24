/*
 * Copyright (c) 2008 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService UP/DOWN functions.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/updown", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_up(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_down(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_up = { "UP", "Grants all access you have permission to on a given channel.", AC_NONE, 1, cs_cmd_up, { .path = "contrib/up" } };
command_t cs_down = { "DOWN", "Removes all current access you posess on a given channel.", AC_NONE, 1, cs_cmd_down, { .path = "contrib/down" } };

void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_up);
	service_named_bind_command("chanserv", &cs_down);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_up);
	service_named_unbind_command("chanserv", &cs_down);
}


static void cs_cmd_up(sourceinfo_t *si, int parc, char *parv[])
{
	chanuser_t *cu;
	mychan_t *mc;
	char *name = parv[0];
	int fl;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "UP");
		command_fail(si, fault_needmoreparams, "Syntax: UP <#channel>");
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

	if (!si->su)
		return; // needs to be done over IRC

	cu = chanuser_find(mc->chan, si->su);

	if (!cu)
	{
		command_fail(si, fault_nosuch_target, "You are not on \2%s\2.", mc->name);
		return;
	}

	fl = chanacs_user_flags(mc, cu->user);

	// Don't check NOOP, because they are explicitly requesting status
	if (ircd->uses_owner)
	{
		if (fl & CA_USEOWNER)
		{
			if (fl & CA_AUTOOP && !(ircd->owner_mode & cu->modes))
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, ircd->owner_mchar[1], CLIENT_NAME(cu->user));
				cu->modes |= ircd->owner_mode;
			}
		}
	}

	if (ircd->uses_protect)
	{
		if (fl & CA_USEPROTECT)
		{
			if (fl & CA_AUTOOP && !(ircd->protect_mode & cu->modes) && !(ircd->uses_owner && cu->modes & ircd->owner_mode))
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, ircd->protect_mchar[1], CLIENT_NAME(cu->user));
				cu->modes |= ircd->protect_mode;
			}
		}
	}

	if (fl & (CA_AUTOOP | CA_OP))
	{
		if (fl & CA_AUTOOP && !(CSTATUS_OP & cu->modes))
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, 'o', CLIENT_NAME(cu->user));
			cu->modes |= CSTATUS_OP;
		}
	}

	if (ircd->uses_halfops)
	{
		if (fl & (CA_AUTOHALFOP | CA_HALFOP))
		{
			if (fl & CA_AUTOHALFOP && !(ircd->halfops_mode & cu->modes))
			{
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, ircd->halfops_mchar[1], CLIENT_NAME(cu->user));
				cu->modes |= ircd->halfops_mode;
			}
		}
	}

	if (fl & (CA_AUTOVOICE | CA_VOICE))
	{
		if (fl & CA_AUTOVOICE && !(CSTATUS_VOICE & cu->modes))
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, 'v', CLIENT_NAME(cu->user));
			cu->modes |= CSTATUS_VOICE;
		}
	}

	command_success_nodata(si, "Upped successfully on \2%s\2.", mc->name);
}


static void cs_cmd_down(sourceinfo_t *si, int parc, char *parv[])
{
	chanuser_t *cu;
	mychan_t *mc;
	char *name = parv[0];

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DOWN");
		command_fail(si, fault_needmoreparams, "Syntax: DOWN <#channel>");
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

	if (!si->su)
		return; // needs to be done over IRC

	cu = chanuser_find(mc->chan, si->su);

	if (!cu)
	{
		command_fail(si, fault_nosuch_target, "You are not on \2%s\2.", mc->name);
		return;
	}

	chanacs_user_flags(mc, cu->user);

	// Don't check NOOP, because they are explicitly requesting status
	if (ircd->uses_owner)
	{
		if (ircd->owner_mode & cu->modes)
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->owner_mchar[1], CLIENT_NAME(cu->user));
			cu->modes &= ~ircd->owner_mode;
		}
	}

	if (ircd->uses_protect)
	{
		if (ircd->protect_mode & cu->modes)
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->protect_mchar[1], CLIENT_NAME(cu->user));
			cu->modes &= ~ircd->protect_mode;
		}
	}

	if ((CSTATUS_OP & cu->modes))
	{
		modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, 'o', CLIENT_NAME(cu->user));
		cu->modes &= ~CSTATUS_OP;
	}

	if (ircd->uses_halfops)
	{
		if (ircd->halfops_mode & cu->modes)
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->halfops_mchar[1], CLIENT_NAME(cu->user));
			cu->modes &= ~ircd->halfops_mode;
		}
	}

	if ((CSTATUS_VOICE & cu->modes))
	{
		modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, 'v', CLIENT_NAME(cu->user));
		cu->modes &= ~CSTATUS_VOICE;
	}

	command_success_nodata(si, "Downed successfully on \2%s\2.", mc->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
