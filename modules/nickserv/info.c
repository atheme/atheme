/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 William Pitcock, et al.
 *
 * This file contains code for the NickServ INFO functions.
 */

#include <atheme.h>

static bool show_custom_metadata = true;

static const char *
ns_obfuscate_time_ago(time_t time)
{
	time_t diff = CURRTIME / SECONDS_PER_WEEK - time / SECONDS_PER_WEEK;
	static char buf[BUFSIZE];

	if (diff <= 1)
		return _("(less than two weeks ago)");
	snprintf(buf, ARRAY_SIZE(buf), _("(about %lu weeks ago)"), (unsigned long)diff);
	return buf;
}

static void
ns_cmd_info(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	struct mynick *mn = NULL;
	struct myuser_name *mun;
	struct user *u = NULL;
	bool recognized = false;
	const char *name = parv[0];
	char buf[BUFSIZE], strfbuf[BUFSIZE], lastlogin[BUFSIZE], *p;
	size_t buflen;
	time_t registered;
	struct tm *tm, *tm2;
	struct metadata *md;
	mowgli_node_t *n;
	mowgli_patricia_iteration_state_t state;
	const char *vhost;
	const char *vhost_timestring;
	const char *vhost_assigner;
	time_t vhost_time;
	bool has_user_auspex;
	bool hide_info;
	struct hook_user_req req;
	struct hook_info_noexist_req noexist_req;

	// On IRC, default the name to something. Not currently documented.
	if (!name && si->su)
	{
		if (!nicksvs.no_nick_ownership)
			name = si->su->nick;
		else if (si->smu)
			name = entity(si->smu)->name;
	}

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INFO");
		command_fail(si, fault_needmoreparams, _("Syntax: INFO <nickname>"));
		return;
	}

	noexist_req.si = si;
	noexist_req.nick = name;

	has_user_auspex = has_priv(si, PRIV_USER_AUSPEX);

	if (!(mu = myuser_find_ext(name)))
	{
		if (has_user_auspex && (mun = myuser_name_find(name)) != NULL)
		{
			const char *setter;
			const char *reason;
			time_t ts;

			md = metadata_find(mun, "private:mark:setter");
			setter = md != NULL ? md->value : "unknown";
			md = metadata_find(mun, "private:mark:reason");
			reason = md != NULL ? md->value : "unknown";
			md = metadata_find(mun, "private:mark:timestamp");
			ts = md != NULL ? atoi(md->value) : 0;

			tm = localtime(&ts);
			strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered anymore, but was marked by %s on %s (%s)."), mun->name, setter, strfbuf, reason);
			hook_call_user_info_noexist(&noexist_req);
		}
		else {
			hook_call_user_info_noexist(&noexist_req);
			command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, name);
		}
		return;
	}

	hide_info = (mu->flags & MU_PRIVATE) && (mu != si->smu);

	if (!nicksvs.no_nick_ownership)
	{
		mn = mynick_find(*name == '=' ? name + 1 : name);
		if (mn != NULL && mn->owner != mu)
			mn = NULL;
		if (mn == NULL)
			mn = mynick_find(entity(mu)->name);
		u = user_find_named(mn->nick);
		if (u != NULL && u->myuser != mu)
		{
			if (myuser_access_verify(u, mu))
				recognized = true;
			u = NULL;
		}
	}

	if (mn != NULL)
		command_success_nodata(si, _("Information on \2%s\2 (account \2%s\2):"), mn->nick, entity(mu)->name);
	else
		command_success_nodata(si, _("Information on \2%s\2:"), entity(mu)->name);

	registered = mn != NULL ? mn->registered : mu->registered;
	tm = localtime(&registered);
	strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);
	command_success_nodata(si, _("Registered : %s (%s ago)"), strfbuf, time_ago(registered));

	// show account's time if it's different from nick's time
	if (mu->registered != registered)
	{
		tm = localtime(&mu->registered);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);
		command_success_nodata(si, _("User reg.  : %s (%s ago)"), strfbuf, time_ago(mu->registered));
	}

	if (config_options.show_entity_id || has_user_auspex)
		command_success_nodata(si, _("Entity ID  : %s"), entity(mu)->id);

	md = metadata_find(mu, "private:usercloak");
	vhost = md ? md->value : NULL;

	md = metadata_find(mu, "private:usercloak-timestamp");
	vhost_timestring = md ? md->value : NULL;

	md = metadata_find(mu, "private:usercloak-assigner");
	vhost_assigner = md ? md->value : NULL;

	if ((!hide_info || has_user_auspex) && (md = metadata_find(mu, "private:host:vhost")))
	{
		mowgli_strlcpy(buf, md->value, sizeof buf);
		if (vhost != NULL)
		{
			p = strchr(buf, '@');
			if (p == NULL)
				p = buf;
			else
				p++;
			mowgli_strlcpy(p, vhost, sizeof buf - (p - buf));
		}
		command_success_nodata(si, _("Last addr  : %s"), buf);
	}

	if ((vhost || vhost_timestring || vhost_assigner) && (si->smu == mu || has_user_auspex))
	{
		buf[0] = '\0';
		buflen = 0;

		if (vhost_timestring && (vhost || has_user_auspex))
		{
			vhost_time = atoi(vhost_timestring);
			tm2 = localtime(&vhost_time);
			strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm2);
			buflen += snprintf(buf + buflen, BUFSIZE - buflen, _(" on %s (%s ago)"), strfbuf, time_ago(vhost_time));
		}

		if (has_user_auspex && vhost_assigner)
			buflen += snprintf(buf + buflen, BUFSIZE - buflen, _(" by %s"), vhost_assigner);

		if (vhost && !buf[0])
			command_success_nodata(si, _("vHost      : %s"), vhost);
		else if (vhost && buf[0])
			command_success_nodata(si, _("vHost      : %s (assigned%s)"), vhost, buf);
		else if (!vhost && buf[0])
			command_success_nodata(si, _("vHost      : unassigned%s"), buf);
	}

  if (!nicksvs.no_nick_ownership)
  {
    if (si->smu == mu || has_user_auspex)
    {
      MOWGLI_ITER_FOREACH(n, mu->nicks.head)
      {
        snprintf(buf, BUFSIZE, "private:usercloak:%s", ((struct mynick *)(n->data))->nick);
        md = metadata_find(mu, buf);
        vhost = md ? md->value : NULL;

        if (vhost)
        {
          command_success_nodata(si, _("vHostNick  : %s (on %s)"), vhost, ((struct mynick *)(n->data))->nick);
          vhost = NULL;
          md = NULL;
        }
      }
    }
  }

	if (has_user_auspex)
	{
		if ((md = metadata_find(mu, "private:host:actual")))
			command_success_nodata(si, _("Real addr  : %s"), md->value);
	}

	if ((!hide_info || has_user_auspex) && recognized)
		command_success_nodata(si, _("Recognized : now (matches access list)"));


	// we have a registered nickname
	if (mn != NULL)
	{
		if (hide_info)
			command_success_nodata(si, _("Last seen  : %s"), ns_obfuscate_time_ago(mn->lastseen));

		// registered nickname is online
		if (u != NULL)
		{
			// it's our nickname or the account isn't Private
			if (!hide_info)
				command_success_nodata(si, _("Last seen  : now"));
			// it's not our nickname and the account is private but we're a soper
			else if (has_user_auspex)
				command_success_nodata(si, _("Last seen  : (hidden) now"));
		}
		// registered nickname is offline
		else
		{
			strftime(lastlogin, sizeof lastlogin, TIME_FORMAT, localtime(&mn->lastseen));
			// it's our nickname or the account isn't private
			if (!hide_info)
				command_success_nodata(si, _("Last seen  : %s (%s ago)"), lastlogin, time_ago(mn->lastseen));
			// it's not our nickname and the account is private but we're a soper
			else if (has_user_auspex)
				command_success_nodata(si, _("Last seen  : (hidden) %s (%s ago)"), lastlogin, time_ago(mn->lastseen));
		}
	}

	if (hide_info)
		command_success_nodata(si, _("User seen  : %s"), ns_obfuscate_time_ago(mu->lastlogin));
	// account is logged in
	if (MOWGLI_LIST_LENGTH(&mu->logins) > 0)
	{
		// it's our account or the account isn't private
		if (!hide_info)
			command_success_nodata(si, _("User seen  : now"));
		// it's not our account and the account is private but we're a soper
		else if (has_user_auspex)
			command_success_nodata(si, _("User seen  : (hidden) now"));
	}
	else
	{
		strftime(lastlogin, sizeof lastlogin, TIME_FORMAT, localtime(&mu->lastlogin));
		// it's our account or the account isn't private
		if (!hide_info)
			command_success_nodata(si, _("User seen  : %s (%s ago)"), lastlogin, time_ago(mu->lastlogin));
		// it's not our account and the account is private but we're a soper
		else if (has_user_auspex)
			command_success_nodata(si, _("User seen  : (hidden) %s (%s ago)"), lastlogin, time_ago(mu->lastlogin));
	}

	// if this is our account or we're a soper, show sessions
	if (mu == si->smu || has_user_auspex)
	{
		buf[0] = '\0';
		MOWGLI_ITER_FOREACH(n, mu->logins.head)
		{
			if (strlen(buf) > 80)
			{
				command_success_nodata(si, _("Logins from: %s"), buf);
				buf[0] = '\0';
			}
			if (buf[0])
				mowgli_strlcat(buf, " ", sizeof buf);
			mowgli_strlcat(buf, ((struct user *)(n->data))->nick, sizeof buf);
		}
		if (buf[0])
			command_success_nodata(si, _("Logins from: %s"), buf);
	}

	if (!nicksvs.no_nick_ownership)
	{
		// if this is our account or we're a soper, list registered nicks
		if (mu == si->smu || has_user_auspex)
		{
			buf[0] = '\0';
			MOWGLI_ITER_FOREACH(n, mu->nicks.head)
			{
				if (strlen(buf) > 80)
				{
					command_success_nodata(si, _("Nicks      : %s"), buf);
					buf[0] = '\0';
				}
				if (buf[0])
					mowgli_strlcat(buf, " ", sizeof buf);
				mowgli_strlcat(buf, ((struct mynick *)(n->data))->nick, sizeof buf);
			}
			if (buf[0])
				command_success_nodata(si, _("Nicks      : %s"), buf);
		}
	}

	if (!(mu->flags & MU_HIDEMAIL)
		|| (si->smu == mu || has_user_auspex))
		command_success_nodata(si, _("Email      : %s%s"), mu->email,
					(mu->flags & MU_HIDEMAIL) ? " (hidden)": "");

	unsigned int mdcount = 0;
	MOWGLI_PATRICIA_FOREACH(md, &state, atheme_object(mu)->metadata)
	{
		if (!strncmp(md->name, "private:", 8))
			continue;
		if (show_custom_metadata)
			command_success_nodata(si, _("Metadata   : %s = %s"),
					md->name, md->value);
		else
			mdcount++;
	}

	if (mdcount && !show_custom_metadata)
	{
		if (module_find_published("nickserv/taxonomy"))
			command_success_nodata(si, ngettext(N_("%u custom metadata entry not shown; use \2/msg %s TAXONOMY %s\2 to view it."), N_("%u custom metadata entries not shown; use \2/msg %s TAXONOMY %s\2 to view them."), mdcount), mdcount, si->service->disp, name);
		else
			command_success_nodata(si, ngettext(N_("%u custom metadata entry not shown."), N_("%u custom metadata entries not shown."), mdcount), mdcount);
	}

	*buf = '\0';

	if (MU_HIDEMAIL & mu->flags)
		strcat(buf, "HideMail");

	if (MU_HOLD & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "Hold");
	}
	if (MU_NEVEROP & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "NeverOp");
	}
	if (MU_NOOP & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "NoOp");
	}
	if (MU_NOMEMO & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "NoMemo");
	}
	if (MU_EMAILMEMOS & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "EMailMemos");
	}
	if (MU_NEVERGROUP & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "NeverGroup");
	}
	if (MU_PRIVATE & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "Private");
	}
	if (MU_REGNOLIMIT & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "RegNoLimit");
	}
    if (MU_NOGREET & mu->flags)
    {
        if (*buf)
            strcat(buf, ", ");

        strcat(buf, "NoGreet");
    }
	if (MU_NOPASSWORD & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "NoPassword");
	}
	if (MU_LOGINNOLIMIT & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "LoginNoLimit");
	}


	if (*buf)
		command_success_nodata(si, _("Flags      : %s"), buf);

#ifdef ENABLE_NLS
	if (mu == si->smu || has_user_auspex)
		command_success_nodata(si, _("Language   : %s"),
				language_get_name(mu->language));
#endif

	if (mu->soper && (mu == si->smu || has_priv(si, PRIV_VIEWPRIVS)))
	{
		command_success_nodata(si, _("Oper class : %s"), mu->soper->operclass ? mu->soper->operclass->name : mu->soper->classname);
	}

	if (mu == si->smu || has_user_auspex || has_priv(si, PRIV_CHAN_AUSPEX))
	{
		struct chanacs *ca;
		unsigned int founder = 0, other = 0;

		MOWGLI_ITER_FOREACH(n, entity(mu)->chanacs.head)
		{
			ca = n->data;
			if (ca->level & CA_FOUNDER)
				founder++;
			else if (ca->level != CA_AKICK)
				other++;
		}
		command_success_nodata(si, _("Channels   : %u founder, %u other"), founder, other);
	}

	if (has_user_auspex && (md = metadata_find(mu, "private:freeze:freezer")))
	{
		const char *setter = md->value;
		const char *reason;
		time_t ts;

		md = metadata_find(mu, "private:freeze:reason");
		reason = md != NULL ? md->value : "unknown";

		md = metadata_find(mu, "private:freeze:timestamp");
		ts = md != NULL ? atoi(md->value) : 0;

		tm = localtime(&ts);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

		command_success_nodata(si, _("%s was \2FROZEN\2 by \2%s\2 on \2%s\2 (%s)."), entity(mu)->name, setter, strfbuf, reason);
	}
	else if (metadata_find(mu, "private:freeze:freezer"))
		command_success_nodata(si, _("%s has been frozen by the %s administration."), entity(mu)->name, me.netname);

	if (has_user_auspex && (md = metadata_find(mu, "private:mark:setter")))
	{
		const char *setter = md->value;
		const char *reason;
		time_t ts;

		md = metadata_find(mu, "private:mark:reason");
		reason = md != NULL ? md->value : "unknown";

		md = metadata_find(mu, "private:mark:timestamp");
		ts = md != NULL ? atoi(md->value) : 0;

		tm = localtime(&ts);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

		command_success_nodata(si, _("%s was \2MARKED\2 by \2%s\2 on \2%s\2 (%s)."), entity(mu)->name, setter, strfbuf, reason);
	}

	if (MU_WAITAUTH & mu->flags)
		command_success_nodata(si, _("%s has \2NOT COMPLETED\2 registration verification."), entity(mu)->name);

	if ((mu == si->smu || has_user_auspex) &&
			(md = metadata_find(mu, "private:verify:emailchg:newemail")))
	{
		const char *newemail = md->value;
		time_t ts;

		md = metadata_find(mu, "private:verify:emailchg:timestamp");
		ts = md != NULL ? atoi(md->value) : 0;
		tm = localtime(&ts);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

		command_success_nodata(si, _("%s has requested an email address change to \2%s\2 on \2%s\2."), entity(mu)->name, newemail, strfbuf);
	}

	req.si = si;
	req.mu = mu;
	req.mn = mn;
	hook_call_user_info(&req);

	command_success_nodata(si, _("*** \2End of Info\2 ***"));

	logcommand(si, CMDLOG_GET, "INFO: \2%s\2", mn != NULL ? mn->nick : entity(mu)->name);
}

static struct command ns_info = {
	.name           = "INFO",
	.desc           = N_("Displays information on registrations."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &ns_cmd_info,
	.help           = { .path = "nickserv/info" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	service_named_bind_command("nickserv", &ns_info);
	add_bool_conf_item("SHOW_CUSTOM_METADATA", &nicksvs.me->conf_table, 0, &show_custom_metadata, true);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_info);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/info", MODULE_UNLOAD_CAPABILITY_OK)
