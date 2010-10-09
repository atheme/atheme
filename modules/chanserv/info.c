/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService INFO functions.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/info", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_info(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_info = { "INFO", N_("Displays information on registrations."),
                        AC_NONE, 2, cs_cmd_info, { .path = "cservice/info" } };

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_info);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_info);
}

static void cs_cmd_info(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char *name = parv[0];
	char buf[BUFSIZE], strfbuf[32];
	struct tm tm;
	myuser_t *mu;
	metadata_t *md;
	mowgli_node_t *n;
	hook_channel_req_t req;
	bool hide_info;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INFO");
		command_fail(si, fault_needmoreparams, _("Syntax: INFO <#channel>"));
		return;
	}

	if (*name != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "INFO");
		command_fail(si, fault_badparams, _("Syntax: INFO <#channel>"));
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), name);
		return;
	}

	if (!has_priv(si, PRIV_CHAN_AUSPEX) && metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 has been closed down by the %s administration."), mc->name, me.netname);
		return;
	}

	hide_info = use_channel_private && mc->flags & MC_PRIVATE &&
		!chanacs_source_has_flag(mc, si, CA_ACLVIEW) &&
		!has_priv(si, PRIV_CHAN_AUSPEX);

	tm = *localtime(&mc->registered);
	strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

	command_success_nodata(si, _("Information on \2%s\2:"), mc->name);

	if (!hide_info)
		command_success_nodata(si, _("Founder    : %s"), mychan_founder_names(mc));

	if (chanacs_source_has_flag(mc, si, CA_ACLVIEW) ||
		has_priv(si, PRIV_CHAN_AUSPEX))
	{
		mu = mychan_pick_successor(mc);
		if (mu != NULL)
			command_success_nodata(si, _("Successor  : %s"), entity(mu)->name);
		else
			command_success_nodata(si, _("Successor  : (none)"));
	}

	command_success_nodata(si, _("Registered : %s (%s ago)"), strfbuf, time_ago(mc->registered));

	if (CURRTIME - mc->used >= 86400)
	{
		if (hide_info)
			command_success_nodata(si, _("Last used  : (about %d weeks ago)"), (int)((CURRTIME - mc->used) / 604800));
		else
		{
			tm = *localtime(&mc->used);
			strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);
			command_success_nodata(si, _("Last used  : %s (%s ago)"), strfbuf, time_ago(mc->used));
		}
	}

	md = metadata_find(mc, "private:mlockext");
	if (mc->mlock_on || mc->mlock_off || mc->mlock_limit || mc->mlock_key || md)
	{
		if (md != NULL && strlen(md->value) > 450)
		{
			/* Be safe */
			command_success_nodata(si, _("Mode lock is too long, not entirely shown"));
			md = NULL;
		}

		command_success_nodata(si, _("Mode lock  : %s"), mychan_get_mlock(mc));
	}


	if ((!hide_info || chanuser_find(mc->chan, si->su)) &&
			(md = metadata_find(mc, "url")))
		command_success_nodata(si, "URL        : %s", md->value);

	if (!hide_info && (md = metadata_find(mc, "email")))
		command_success_nodata(si, "Email      : %s", md->value);

	if (!hide_info)
	{
		MOWGLI_ITER_FOREACH(n, object(mc)->metadata.head)
		{
			md = n->data;
			if (md->private)
				continue;
			/* these are shown separately */
			if (!strcasecmp(md->name, "email") ||
					!strcasecmp(md->name, "url") ||
					!strcasecmp(md->name, "disable_fantasy"))
				continue;
			command_success_nodata(si, _("Metadata   : %s = %s"),
					md->name, md->value);
		}
	}

	*buf = '\0';

	if (MC_HOLD & mc->flags)
		strcat(buf, "HOLD");

	if (MC_SECURE & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "SECURE");
	}

	if (MC_VERBOSE & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "VERBOSE");
	}
	if (MC_VERBOSE_OPS & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "VERBOSE_OPS");
	}

	if (MC_RESTRICTED & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "RESTRICTED");
	}

	if (MC_KEEPTOPIC & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "KEEPTOPIC");
	}

	if (MC_TOPICLOCK & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "TOPICLOCK");
	}

	if (MC_GUARD & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "GUARD");
	}

	if (chansvs.fantasy && !metadata_find(mc, "disable_fantasy"))
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "FANTASY");
	}

	if (use_channel_private && MC_PRIVATE & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "PRIVATE");
	}

	if (use_limitflags && MC_LIMITFLAGS & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "LIMITFLAGS");
	}

	if (*buf)
		command_success_nodata(si, _("Flags      : %s"), buf);

	if ((md = metadata_find(mc, "private:prefix")))
		command_success_nodata(si, _("Prefix     : %s"), md->value);
	else
		command_success_nodata(si, _("Prefix     : %s (default)"), chansvs.trigger);

	if (has_priv(si, PRIV_CHAN_AUSPEX) && (md = metadata_find(mc, "private:mark:setter")))
	{
		const char *setter = md->value;
		const char *reason;
		time_t ts;

		md = metadata_find(mc, "private:mark:reason");
		reason = md != NULL ? md->value : "unknown";

		md = metadata_find(mc, "private:mark:timestamp");
		ts = md != NULL ? atoi(md->value) : 0;

		tm = *localtime(&ts);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		command_success_nodata(si, _("%s was \2MARKED\2 by %s on %s (%s)"), mc->name, setter, strfbuf, reason);
	}

	if (has_priv(si, PRIV_CHAN_AUSPEX) && (MC_INHABIT & mc->flags))
		command_success_nodata(si, _("%s is temporarily holding this channel."), chansvs.nick);

	if (has_priv(si, PRIV_CHAN_AUSPEX) && (md = metadata_find(mc, "private:close:closer")))
	{
		const char *setter = md->value;
		const char *reason;
		time_t ts;

		md = metadata_find(mc, "private:close:reason");
		reason = md != NULL ? md->value : "unknown";

		md = metadata_find(mc, "private:close:timestamp");
		ts = md != NULL ? atoi(md->value) : 0;

		tm = *localtime(&ts);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		command_success_nodata(si, _("%s was \2CLOSED\2 by %s on %s (%s)"), mc->name, setter, strfbuf, reason);
	}

	req.mc = mc;
	req.si = si;
	hook_call_channel_info(&req);

	command_success_nodata(si, _("\2*** End of Info ***\2"));
	logcommand(si, CMDLOG_GET, "INFO: \2%s\2", mc->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
