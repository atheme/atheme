/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService DROP function.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/drop", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_drop(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_fdrop(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_drop = { "DROP", N_("Drops a channel registration."),
                        AC_NONE, 2, cs_cmd_drop, { .path = "cservice/drop" } };
command_t cs_fdrop = { "FDROP", N_("Forces dropping of a channel registration."),
                        PRIV_CHAN_ADMIN, 1, cs_cmd_fdrop, { .path = "cservice/fdrop" } };

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_drop);
        service_named_bind_command("chanserv", &cs_fdrop);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_drop);
	service_named_unbind_command("chanserv", &cs_fdrop);
}

static void create_challenge(sourceinfo_t *si, const char *name, int v, char *dest)
{
	char buf[256];
	int digest[4];
	md5_state_t ctx;

	snprintf(buf, sizeof buf, "%lu:%s:%s",
			(unsigned long)(CURRTIME / 300) - v,
			get_source_name(si),
			name);
	md5_init(&ctx);
	md5_append(&ctx, (unsigned char *)buf, strlen(buf));
	md5_finish(&ctx, (unsigned char *)digest);
	/* note: this depends on byte order, but that's ok because
	 * it's only going to work in the same atheme instance anyway
	 */
	snprintf(dest, 80, "%x:%x", digest[0], digest[1]);
}

static void cs_cmd_drop(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char *name = parv[0];
	char *key = parv[1];
	char fullcmd[512];
	char key0[80], key1[80];

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		command_fail(si, fault_needmoreparams, _("Syntax: DROP <#channel>"));
		return;
	}

	if (*name != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DROP");
		command_fail(si, fault_badparams, _("Syntax: DROP <#channel>"));
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), name);
		return;
	}

	if (si->c != NULL)
	{
		command_fail(si, fault_noprivs, _("For security reasons, you may not drop a channel registration with a fantasy command."));
		return;
	}

	if (!is_founder(mc, entity(si->smu)))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		logcommand(si, CMDLOG_REGISTER, "DROP: \2%s\2 failed to drop (closed)", mc->name);
		command_fail(si, fault_noprivs, _("The channel \2%s\2 is closed; it cannot be dropped."), mc->name);
		return;
	}

	if (mc->flags & MC_HOLD)
	{
		command_fail(si, fault_noprivs, _("The channel \2%s\2 is held; it cannot be dropped."), mc->name);
		return;
	}

	if (si->su != NULL)
	{
		if (!key)
		{
			create_challenge(si, mc->name, 0, key0);
			snprintf(fullcmd, sizeof fullcmd, "/%s%s DROP %s %s",
					(ircd->uses_rcommand == false) ? "msg " : "",
					chansvs.me->disp, mc->name, key0);
			command_success_nodata(si, _("To avoid accidental use of this command, this operation has to be confirmed. Please confirm by replying with \2%s\2"),
					fullcmd);
			return;
		}
		/* accept current and previous key */
		create_challenge(si, mc->name, 0, key0);
		create_challenge(si, mc->name, 1, key1);
		if (strcmp(key, key0) && strcmp(key, key1))
		{
			command_fail(si, fault_badparams, _("Invalid key for %s."), "DROP");
			return;
		}
	}

	logcommand(si, CMDLOG_REGISTER, "DROP: \2%s\2", mc->name);

	hook_call_channel_drop(mc);
	if (mc->chan != NULL && !(mc->chan->flags & CHAN_LOG))
		part(mc->name, chansvs.nick);
	object_unref(mc);
	command_success_nodata(si, _("The channel \2%s\2 has been dropped."), name);
	return;
}

static void cs_cmd_fdrop(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char *name = parv[0];

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FDROP");
		command_fail(si, fault_needmoreparams, _("Syntax: FDROP <#channel>"));
		return;
	}

	if (*name != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "FDROP");
		command_fail(si, fault_badparams, _("Syntax: FDROP <#channel>"));
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), name);
		return;
	}

	if (si->c != NULL)
	{
		command_fail(si, fault_noprivs, _("For security reasons, you may not drop a channel registration with a fantasy command."));
		return;
	}

	if (mc->flags & MC_HOLD)
	{
		command_fail(si, fault_noprivs, _("The channel \2%s\2 is held; it cannot be dropped."), mc->name);
		return;
	}

	logcommand(si, CMDLOG_ADMIN | LG_REGISTER, "FDROP: \2%s\2", mc->name);
	wallops("%s dropped the channel \2%s\2", get_oper_name(si), name);

	hook_call_channel_drop(mc);
	if (mc->chan != NULL && !(mc->chan->flags & CHAN_LOG))
		part(mc->name, chansvs.nick);
	object_unref(mc);
	command_success_nodata(si, _("The channel \2%s\2 has been dropped."), name);
	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
