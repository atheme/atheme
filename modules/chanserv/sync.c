/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SYNC functions.
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
                        AC_NONE, 1, cs_cmd_sync, { .path = "cservice/sync" } };

static bool no_vhost_sync = false;

static void do_chanuser_sync(mychan_t *mc, chanuser_t *cu, chanacs_t *ca,
		bool take)
{
	char akickreason[120] = "User is banned from this channel", *p;
	int fl;
	bool noop;

	if (is_internal_client(cu->user))
		return;

	if (ca != NULL && ca->entity != NULL && cu->user->myuser != NULL)
	{
		entity_chanacs_validation_vtable_t *vt;

		vt = myentity_get_chanacs_validator(ca->entity);
		if (vt->match_entity(ca, entity(cu->user->myuser)) == NULL)
			return;
	}
	fl = chanacs_user_flags(mc, cu->user);
	noop = mc->flags & MC_NOOP || (cu->user->myuser != NULL &&
			cu->user->myuser->flags & MU_NOOP);

	slog(LG_DEBUG, "do_chanuser_sync(): flags: %s, noop: %s", bitmask_to_flags(fl), noop ? "true" : "false");

	if (take && !(fl & CA_ALLPRIVS) && (mc->flags & MC_RESTRICTED) && !has_priv_user(cu->user, PRIV_JOIN_STAFFONLY))
	{
		/* Stay on channel if this would empty it -- jilles */
		if (mc->chan->nummembers <= (mc->flags & MC_GUARD ? 2 : 1))
		{
			mc->flags |= MC_INHABIT;
			if (!(mc->flags & MC_GUARD))
				join(mc->chan->name, chansvs.nick);
		}

		if (mc->mlock_on & CMODE_INVITE || mc->chan->modes & CMODE_INVITE)
		{
			if (!(mc->chan->modes & CMODE_INVITE))
				check_modes(mc, true);
			remove_banlike(chansvs.me->me, mc->chan, ircd->invex_mchar, cu->user);
			modestack_flush_channel(mc->chan);
		}
		else
		{
			ban(chansvs.me->me, mc->chan, cu->user);
			remove_ban_exceptions(chansvs.me->me, mc->chan, cu->user);
		}

		try_kick(chansvs.me->me, mc->chan, cu->user, "You are not authorized to be on this channel");
		return;
	}
	if (fl & CA_AKICK && !(fl & CA_EXEMPT))
	{
		chanacs_t *ca2;
		metadata_t *md;

		/* Stay on channel if this would empty it -- jilles */
		if (mc->chan->nummembers <= (mc->flags & MC_GUARD ? 2 : 1))
		{
			mc->flags |= MC_INHABIT;
			if (!(mc->flags & MC_GUARD))
				join(mc->chan->name, chansvs.nick);
		}

		/* use a user-given ban mask if possible -- jilles */
		ca2 = chanacs_find_host_by_user(mc, cu->user, CA_AKICK);
		if (ca2 != NULL)
		{
			if (chanban_find(mc->chan, ca2->host, 'b') == NULL)
			{
				chanban_add(mc->chan, ca2->host, 'b');
				modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, 'b', ca2->host);

				/* ban immediately */
				modestack_flush_channel(mc->chan);
			}
		}
		else if (cu->user->myuser != NULL)
		{
			/* XXX this could be done more efficiently */
			ca2 = chanacs_find(mc, entity(cu->user->myuser), CA_AKICK);
			ban(chansvs.me->me, mc->chan, cu->user);
		}

		remove_ban_exceptions(chansvs.me->me, mc->chan, cu->user);
		if (ca2 != NULL)
		{
			md = metadata_find(ca2, "reason");
			if (md != NULL && *md->value != '|')
			{
				snprintf(akickreason, sizeof akickreason,
						"Banned: %s", md->value);
				p = strchr(akickreason, '|');
				if (p != NULL)
					*p = '\0';
				else
					p = akickreason + strlen(akickreason);

				/* strip trailing spaces, so as not to
				 * disclose the existence of an oper reason */
				p--;
				while (p > akickreason && *p == ' ')
					p--;
				p[1] = '\0';
			}
		}

		try_kick(chansvs.me->me, mc->chan, cu->user, akickreason);
		return;
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
		else if ((take || mc->flags & MC_SECURE) &&
				ircd->owner_mode & cu->modes)
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
		else if ((take || mc->flags & MC_SECURE) &&
				ircd->protect_mode & cu->modes)
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

		if (cu->modes & CSTATUS_OP)
			return;
	}
	else if ((take || mc->flags & MC_SECURE) && CSTATUS_OP & cu->modes)
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

			if (cu->modes & ircd->halfops_mode)
				return;
		}
		else if ((take || mc->flags & MC_SECURE) &&
				ircd->halfops_mode & cu->modes)
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
			return;
		}
	}
	else if (take && CSTATUS_VOICE & cu->modes)
	{
		modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, 'v', CLIENT_NAME(cu->user));
		cu->modes &= ~CSTATUS_VOICE;
	}
}

void do_channel_sync(mychan_t *mc, chanacs_t *ca)
{
	chanuser_t *cu;
	mowgli_node_t *n, *tn;

	return_if_fail(mc != NULL);
	if (mc->chan == NULL)
		return;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chan->members.head)
	{
		cu = (chanuser_t *)n->data;

		do_chanuser_sync(mc, cu, ca, true);
	}
}

/* this could be a little slow, should probably have an option to disable it */
static void sync_user(user_t *u)
{
	mowgli_node_t *iter;

	return_if_fail(u != NULL);

	if (no_vhost_sync)
		return;

	MOWGLI_ITER_FOREACH(iter, u->channels.head)
	{
		chanuser_t *cu = iter->data;
		mychan_t *mc;

		mc = MYCHAN_FROM(cu->chan);
		if (mc == NULL)
			continue;

		do_chanuser_sync(mc, cu, NULL, !(mc->flags & MC_NOSYNC));
	}

	if (u->myuser != NULL)
		hook_call_grant_channel_access(u);
}

static void sync_myuser(myuser_t *mu)
{
	mowgli_node_t *iter;

	return_if_fail(mu != NULL);

	MOWGLI_ITER_FOREACH(iter, mu->logins.head)
	{
		sync_user(iter->data);
	}
}

static void sync_channel_acl_change(hook_channel_acl_req_t *hookdata)
{
	mychan_t *mc;

	return_if_fail(hookdata != NULL);
	return_if_fail(hookdata->ca != NULL);

	mc = hookdata->ca->mychan;
	return_if_fail(mc != NULL);

	if (MC_NOSYNC & mc->flags)
		return;

	if (((hookdata->ca->level ^ hookdata->oldlevel) &
	    (CA_AKICK | CA_EXEMPT | CA_USEOWNER | CA_USEPROTECT | CA_AUTOOP |
	     CA_OP | CA_AUTOHALFOP | CA_HALFOP | CA_AUTOVOICE | CA_VOICE)) == 0)
		return;

	do_channel_sync(mc, hookdata->ca);
}

static void cs_cmd_sync(sourceinfo_t *si, int parc, char *parv[])
{
	char *name = parv[0];
	mychan_t *mc;

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

	do_channel_sync(mc, NULL);

	command_success_nodata(si, "Sync complete for \2%s\2.", mc->name);
}

static void cs_cmd_set_nosync(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_set_nosync = { "NOSYNC", N_("Disables automatic channel ACL syncing."), AC_NONE, 2, cs_cmd_set_nosync, { .path = "cservice/set_nosync" } };

mowgli_patricia_t **cs_set_cmdtree;

static void cs_cmd_set_nosync(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET NOSYNC");
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		if (MC_NOSYNC & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "NOSYNC", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOSYNC:ON: \2%s\2", mc->name);

		mc->flags |= MC_NOSYNC;

		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "NOSYNC", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!(MC_NOSYNC & mc->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "NOSYNC", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOSYNC:OFF: \2%s\2", mc->name);

		mc->flags &= ~MC_NOSYNC;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "NOSYNC", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NOSYNC");
		return;
	}
}

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree");
	service_named_bind_command("chanserv", &cs_sync);

	command_add(&cs_set_nosync, *cs_set_cmdtree);

	add_bool_conf_item("NO_VHOST_SYNC", &chansvs.me->conf_table, 0, &no_vhost_sync, false);

	hook_add_event("channel_acl_change");
	hook_add_channel_acl_change(sync_channel_acl_change);

	hook_add_event("user_sethost");
	hook_add_user_sethost(sync_user);

	hook_add_event("user_oper");
	hook_add_user_oper(sync_user);

	hook_add_event("user_identify");
	hook_add_user_identify(sync_user);

	hook_add_event("user_register");
	hook_add_user_register(sync_myuser);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_channel_acl_change(sync_channel_acl_change);
	hook_del_user_sethost(sync_user);
	hook_del_user_oper(sync_user);
	hook_del_user_identify(sync_user);

	service_named_unbind_command("chanserv", &cs_sync);
	command_delete(&cs_set_nosync, *cs_set_cmdtree);

	del_conf_item("NO_VHOST_SYNC", &chansvs.me->conf_table);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
