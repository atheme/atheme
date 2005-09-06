/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ INFO functions.
 *
 * $Id: info.c 2179 2005-09-06 09:17:45Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/info", FALSE, _modinit, _moddeinit,
	"$Id: info.c 2179 2005-09-06 09:17:45Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_info(char *origin);

command_t ns_info = { "INFO", "Displays information on registrations.", AC_NONE, ns_cmd_info };

list_t *ns_cmdtree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	command_add(&ns_info, ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_info, ns_cmdtree);
}

static void ns_cmd_info(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	char *name = strtok(NULL, " ");
	char buf[BUFSIZE], strfbuf[32];
	struct tm tm;
	metadata_t *md;
	node_t *n;

	if (!name)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters specified for \2INFO\2.");
		notice(nicksvs.nick, origin, "Syntax: INFO <nickname>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	snoop("INFO: \2%s\2 by \2%s\2", name, origin);

	tm = *localtime(&mu->registered);
	strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

	notice(nicksvs.nick, origin, "Information on \2%s\2:", mu->name);

	notice(nicksvs.nick, origin, "Registered : %s (%s ago)", strfbuf, time_ago(mu->registered));

	if (!(mu->flags & MU_HIDEMAIL)
		|| (is_sra(u->myuser) || is_ircop(u) || u->myuser == mu))
		notice(nicksvs.nick, origin, "Email      : %s%s", mu->email,
					(mu->flags & MU_HIDEMAIL) ? " (hidden)": "");

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
	if (MU_ALIAS & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "Alias");
	}

	if (*buf)
		notice(nicksvs.nick, origin, "Flags      : %s", buf);

	if ((mu->flags & MU_ALIAS) && (md = metadata_find(mu, METADATA_USER, "private:alias:parent")))
		notice(nicksvs.nick, origin, "Parent     : %s", md->value);

	if (LIST_LENGTH(&mu->logins) == 0)
		notice(nicksvs.nick, origin, "Logins from: <none>");
	else if (mu == u->myuser || is_ircop(u) || is_sra(u->myuser))
	{
		buf[0] = '\0';
		LIST_FOREACH(n, mu->logins.head)
		{
			if (strlen(buf) > 80)
			{
				notice(nicksvs.nick, origin, "Logins from: %s", buf);
				buf[0] = '\0';
			}
			if (buf[0])
				strcat(buf, " ");
			strcat(buf, ((user_t *)(n->data))->nick);
		}
		if (buf[0])
			notice(nicksvs.nick, origin, "Logins from: %s", buf);
	}
	else
		notice(nicksvs.nick, origin, "Logins from: <hidden>");

        if ((is_ircop(u) || is_sra(u->myuser)) && (md = metadata_find(mu, METADATA_USER, "private:freeze:freezer")))
        {
                char *setter = md->value;
                char *reason;
                time_t ts;

                md = metadata_find(mu, METADATA_USER, "private:freeze:reason");
                reason = md->value;

                md = metadata_find(mu, METADATA_USER, "private:freeze:timestamp");
                ts = atoi(md->value);

                tm = *localtime(&ts);
                strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

                notice(nicksvs.nick, origin, "%s was \2FROZEN\2 by %s on %s (%s)", mu->name, setter, strfbuf, reason);
        }

	if ((is_ircop(u) || is_sra(u->myuser)) && (md = metadata_find(mu, METADATA_USER, "private:mark:setter")))
	{
		char *setter = md->value;
		char *reason;
		time_t ts;

		md = metadata_find(mu, METADATA_USER, "private:mark:reason");
		reason = md->value;

		md = metadata_find(mu, METADATA_USER, "private:mark:timestamp");
		ts = atoi(md->value);

		tm = *localtime(&ts);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		notice(nicksvs.nick, origin, "%s was \2MARKED\2 by %s on %s (%s)", mu->name, setter, strfbuf, reason);
	}

	if ((MU_WAITAUTH & mu->flags) && (is_ircop(u) || is_sra(u->myuser)))
		notice(nicksvs.nick, origin, "%s has not completed registration verification", mu->name);

	notice(nicksvs.nick, origin, "*** \2End of Info\2 ***");
}
