/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService INFO functions.
 *
 * $Id: info.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/info", FALSE, _modinit, _moddeinit,
	"$Id: info.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_info(char *origin);

command_t cs_info = { "INFO", "Displays information on registrations.",
                        AC_NONE, cs_cmd_info };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_info, cs_cmdtree);
	help_addentry(cs_helptree, "INFO", "help/cservice/info", NULL);
}

void _moddeinit()
{
	command_delete(&cs_info, cs_cmdtree);
	help_delentry(cs_helptree, "INFO");
}

static void cs_cmd_info(char *origin)
{
	user_t *u = user_find_named(origin);
	mychan_t *mc;
	char *name = strtok(NULL, " ");
	char buf[BUFSIZE], strfbuf[32];
	struct tm tm;
	metadata_t *md;
	hook_channel_req_t req;
	char *p, *q, *qq;
	int dir;

	if (!name)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "INFO");
		notice(chansvs.nick, origin, "Syntax: INFO <#channel>");
		return;
	}

	if (*name != '#')
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "INFO");
		notice(chansvs.nick, origin, "Syntax: INFO <#channel>");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (!has_priv(u, PRIV_CHAN_AUSPEX) && (md = metadata_find(mc, METADATA_CHANNEL, "private:close:closer")))
	{
		notice(chansvs.nick, origin, "\2%s\2 has been closed down by the %s administration.", mc->name, me.netname);
		return;
	}

	tm = *localtime(&mc->registered);
	strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

	notice(chansvs.nick, origin, "Information on \2%s\2:", mc->name);

	if (LIST_LENGTH(&mc->founder->logins))
		notice(chansvs.nick, origin, "Founder    : %s (logged in)", mc->founder->name);
	else
		notice(chansvs.nick, origin, "Founder    : %s (not logged in)", mc->founder->name);

	notice(chansvs.nick, origin, "Registered : %s (%s ago)", strfbuf, time_ago(mc->registered));

	if (CURRTIME - mc->used >= 86400)
	{
		tm = *localtime(&mc->used);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);
		notice(chansvs.nick, origin, "Last used  : %s (%s ago)", strfbuf, time_ago(mc->used));
	}

	md = metadata_find(mc, METADATA_CHANNEL, "private:mlockext");
	if (mc->mlock_on || mc->mlock_off || mc->mlock_limit || mc->mlock_key || md)
	{
		char params[BUFSIZE];

		if (md != NULL && strlen(md->value) > 450)
		{
			/* Be safe */
			notice(chansvs.nick, origin, "Mode lock is too long, not entirely shown");
			md = NULL;
		}

		*buf = 0;
		*params = 0;
		dir = MTYPE_NUL;

		if (mc->mlock_on)
		{
			if (dir != MTYPE_ADD)
				dir = MTYPE_ADD, strcat(buf, "+");
			strcat(buf, flags_to_string(mc->mlock_on));

		}

		if (mc->mlock_limit)
		{
			if (dir != MTYPE_ADD)
				dir = MTYPE_ADD, strcat(buf, "+");
			strcat(buf, "l");
			strcat(params, " ");
			strcat(params, itoa(mc->mlock_limit));
		}

		if (mc->mlock_key)
		{
			if (dir != MTYPE_ADD)
				dir = MTYPE_ADD, strcat(buf, "+");
			strcat(buf, "k");
			strcat(params, " *");
		}

		if (md)
		{
			p = md->value;
			q = buf + strlen(buf);
			while (*p != '\0')
			{
				if (p[1] != ' ' && p[1] != '\0')
				{
					if (dir != MTYPE_ADD)
						dir = MTYPE_ADD, *q++ = '+';
					*q++ = *p++;
					strcat(params, " ");
					qq = params + strlen(params);
					while (*p != '\0' && *p != ' ')
						*qq++ = *p++;
					*qq = '\0';
				}
				else
				{
					p++;
					while (*p != '\0' && *p != ' ')
						p++;
				}
				while (*p == ' ')
					p++;
			}
			*q = '\0';
		}

		if (mc->mlock_off)
		{
			if (dir != MTYPE_DEL)
				dir = MTYPE_DEL, strcat(buf, "-");
			strcat(buf, flags_to_string(mc->mlock_off));
			if (mc->mlock_off & CMODE_LIMIT)
				strcat(buf, "l");
			if (mc->mlock_off & CMODE_KEY)
				strcat(buf, "k");
		}

		if (md)
		{
			p = md->value;
			q = buf + strlen(buf);
			while (*p != '\0')
			{
				if (p[1] == ' ' || p[1] == '\0')
				{
					if (dir != MTYPE_DEL)
						dir = MTYPE_DEL, *q++ = '-';
					*q++ = *p;
				}
				p++;
				while (*p != '\0' && *p != ' ')
					p++;
				while (*p == ' ')
					p++;
			}
			*q = '\0';
		}

		if (*buf)
			notice(chansvs.nick, origin, "Mode lock  : %s%s", buf, (params) ? params : "");
	}


	if ((md = metadata_find(mc, METADATA_CHANNEL, "url")))
		notice(chansvs.nick, origin, "URL        : %s", md->value);

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

	if (MC_STAFFONLY & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "STAFFONLY");
	}

	if (MC_KEEPTOPIC & mc->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "KEEPTOPIC");
	}

	if (*buf)
		notice(chansvs.nick, origin, "Flags      : %s", buf);

	if (has_priv(u, PRIV_CHAN_AUSPEX) && (md = metadata_find(mc, METADATA_CHANNEL, "private:mark:setter")))
	{
		char *setter = md->value;
		char *reason;
		time_t ts;

		md = metadata_find(mc, METADATA_CHANNEL, "private:mark:reason");
		reason = md != NULL ? md->value : "unknown";

		md = metadata_find(mc, METADATA_CHANNEL, "private:mark:timestamp");
		ts = md != NULL ? atoi(md->value) : 0;

		tm = *localtime(&ts);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		notice(chansvs.nick, origin, "%s was \2MARKED\2 by %s on %s (%s)", mc->name, setter, strfbuf, reason);
	}

	if (has_priv(u, PRIV_CHAN_AUSPEX) && (MC_INHABIT & mc->flags))
		notice(chansvs.nick, origin, "%s is temporally holding this channel.", chansvs.nick);

	if (has_priv(u, PRIV_CHAN_AUSPEX) && (md = metadata_find(mc, METADATA_CHANNEL, "private:close:closer")))
	{
		char *setter = md->value;
		char *reason;
		time_t ts;

		md = metadata_find(mc, METADATA_CHANNEL, "private:close:reason");
		reason = md != NULL ? md->value : "unknown";

		md = metadata_find(mc, METADATA_CHANNEL, "private:close:timestamp");
		ts = md != NULL ? atoi(md->value) : 0;

		tm = *localtime(&ts);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		notice(chansvs.nick, origin, "%s was \2CLOSED\2 by %s on %s (%s)", mc->name, setter, strfbuf, reason);
	}

	req.u = u;
	req.mc = mc;
	hook_call_event("channel_info", &req);

	notice(chansvs.nick,origin, "\2*** End of Info ***\2");
	logcommand(chansvs.me, u, CMDLOG_GET, "%s INFO", mc->name);
}
