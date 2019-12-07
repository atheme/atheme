/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService INFO functions.
 */

#include <atheme.h>

static bool show_custom_metadata = true;

static void
cs_cmd_info(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	char *name = parv[0];
	char buf[BUFSIZE], strfbuf[BUFSIZE];
	struct tm *tm;
	struct myuser *mu;
	struct metadata *md;
	mowgli_patricia_iteration_state_t state;
	struct hook_channel_req req;
	bool hide_info, hide_acl;

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
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, name);
		return;
	}

	if (!has_priv(si, PRIV_CHAN_AUSPEX) && metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 has been closed down by the %s administration."), mc->name, me.netname);
		return;
	}

	hide_info = (mc->flags & MC_PRIVATE) && !chanacs_source_has_flag(mc, si, CA_ACLVIEW) &&
		!has_priv(si, PRIV_CHAN_AUSPEX);

	hide_acl = !chanacs_source_has_flag(mc, si, CA_ACLVIEW) &&
		!has_priv(si, PRIV_CHAN_AUSPEX) &&
		!(mc->flags & MC_PUBACL);

	tm = localtime(&mc->registered);
	strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

	command_success_nodata(si, _("Information on \2%s\2:"), mc->name);

	if (!(hide_info && hide_acl))
		command_success_nodata(si, _("Founder    : %s"), mychan_founder_names(mc));

	if (!hide_acl)
	{
		mu = mychan_pick_successor(mc);
		if (mu != NULL)
			command_success_nodata(si, _("Successor  : %s"), entity(mu)->name);
		else
			command_success_nodata(si, _("Successor  : (none)"));
	}

	command_success_nodata(si, _("Registered : %s (%s ago)"), strfbuf, time_ago(mc->registered));

	if ((CURRTIME - mc->used) >= SECONDS_PER_DAY)
	{
		if (hide_info)
			command_success_nodata(si, _("Last used  : (about %u week(s) ago)"), (unsigned int)((CURRTIME - mc->used) / SECONDS_PER_WEEK));
		else
		{
			tm = localtime(&mc->used);
			strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);
			command_success_nodata(si, _("Last used  : %s (%s ago)"), strfbuf, time_ago(mc->used));
		}
	}

	md = metadata_find(mc, "private:mlockext");
	if (mc->mlock_on || mc->mlock_off || mc->mlock_limit || mc->mlock_key || md)
	{
		if (md != NULL && strlen(md->value) > 450)
		{
			// Be safe
			command_success_nodata(si, _("Mode lock is too long, not entirely shown"));
			md = NULL;
		}

		command_success_nodata(si, _("Mode lock  : %s"), mychan_get_mlock(mc));
	}


	if ((!hide_info || (si->su != NULL && chanuser_find(mc->chan, si->su))) &&
			(md = metadata_find(mc, "url")))
		command_success_nodata(si, "URL        : %s", md->value);

	if (!hide_info && (md = metadata_find(mc, "email")))
		command_success_nodata(si, "Email      : %s", md->value);

	if ((!hide_info || (si->su != NULL && chanuser_find(mc->chan, si->su))) &&
			(md = metadata_find(mc, "private:entrymsg")))
		command_success_nodata(si, "Entrymsg   : %s", md->value);

	if (!hide_info)
	{
		unsigned int mdcount = 0;

		MOWGLI_PATRICIA_FOREACH(md, &state, atheme_object(mc)->metadata)
		{
			if (!strncmp(md->name, "private:", 8))
				continue;
			// these are shown separately
			if (!strcasecmp(md->name, "email") ||
					!strcasecmp(md->name, "url") ||
					!strcasecmp(md->name, "disable_fantasy"))
				continue;
			if (show_custom_metadata)
				command_success_nodata(si, _("Metadata   : %s = %s"),
						md->name, md->value);
			else
				mdcount++;
		}

		if (mdcount && !show_custom_metadata)
		{
			command_success_nodata(si, ngettext(N_("%u custom metadata entry not shown."),
			                                    N_("%u custom metadata entries not shown."),
			                                    mdcount), mdcount);

			if (module_find_published("chanserv/taxonomy"))
				command_success_nodata(si, ngettext(N_("Use \2/msg %s TAXONOMY %s\2 to view it."),
				                                    N_("Use \2/msg %s TAXONOMY %s\2 to view them."),
				                                    mdcount), si->service->disp, name);
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

	if (MC_ANTIFLOOD & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "ANTIFLOOD");
	}

	if (chansvs.fantasy && !metadata_find(mc, "disable_fantasy"))
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "FANTASY");
	}

	if (MC_NOSYNC & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "NOSYNC");
	}

	if (MC_PRIVATE & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "PRIVATE");
	}

	if (MC_PUBACL & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "PUBACL");
	}

	if (use_limitflags && MC_LIMITFLAGS & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "LIMITFLAGS");
	}

	if (*buf)
		command_success_nodata(si, _("Flags      : %s"), buf);

	if (chansvs.fantasy && !metadata_find(mc, "disable_fantasy"))
	{
		if ((md = metadata_find(mc, "private:prefix")))
			command_success_nodata(si, _("Prefix     : %s"), md->value);
		else
			command_success_nodata(si, _("Prefix     : %s (default)"), chansvs.trigger);
	}

	if (!hide_info && (md = metadata_find(mc, "private:antiflood:enforce-method")))
	{
		if ((md = metadata_find(mc, "private:antiflood:enforce-method")))
			command_success_nodata(si, _("AntiFlood  : %s"), md->value);
		else
			command_success_nodata(si, _("AntiFlood  : (default)"));
	}


	if (has_priv(si, PRIV_CHAN_AUSPEX) && (md = metadata_find(mc, "private:mark:setter")))
	{
		const char *setter = md->value;
		const char *reason;
		time_t ts;

		md = metadata_find(mc, "private:mark:reason");
		reason = md != NULL ? md->value : "unknown";

		md = metadata_find(mc, "private:mark:timestamp");
		ts = md != NULL ? atoi(md->value) : 0;

		tm = localtime(&ts);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

		command_success_nodata(si, _("%s was \2MARKED\2 by \2%s\2 on \2%s\2 (%s)."), mc->name, setter, strfbuf, reason);
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

		tm = localtime(&ts);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

		command_success_nodata(si, _("%s was \2CLOSED\2 by %s on %s (%s)"), mc->name, setter, strfbuf, reason);
	}

	req.mc = mc;
	req.si = si;
	hook_call_channel_info(&req);

	command_success_nodata(si, _("\2*** End of Info ***\2"));
	logcommand(si, CMDLOG_GET, "INFO: \2%s\2", mc->name);
}

static struct command cs_info = {
	.name           = "INFO",
	.desc           = N_("Displays information on registrations."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_info,
	.help           = { .path = "cservice/info" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	service_named_bind_command("chanserv", &cs_info);
	add_bool_conf_item("SHOW_CUSTOM_METADATA", &chansvs.me->conf_table, 0, &show_custom_metadata, true);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_info);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/info", MODULE_UNLOAD_CAPABILITY_OK)
