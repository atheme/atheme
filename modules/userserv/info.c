/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ INFO functions.
 *
 * $Id: info.c 3721 2005-11-09 06:13:52Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/info", FALSE, _modinit, _moddeinit,
	"$Id: info.c 3721 2005-11-09 06:13:52Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_info(char *origin);

command_t us_info = { "INFO", "Displays information on registrations.", AC_NONE, us_cmd_info };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	command_add(&us_info, us_cmdtree);
	help_addentry(us_helptree, "INFO", "help/userserv/info", NULL);
}

void _moddeinit()
{
	command_delete(&us_info, us_cmdtree);
	help_delentry(us_helptree, "INFO");
}

static void us_cmd_info(char *origin)
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
		notice(usersvs.nick, origin, "Insufficient parameters specified for \2INFO\2.");
		notice(usersvs.nick, origin, "Syntax: INFO <account>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	snoop("INFO: \2%s\2 by \2%s\2", name, origin);

	tm = *localtime(&mu->registered);
	strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

	notice(usersvs.nick, origin, "Information on \2%s\2:", mu->name);

	if ((is_ircop(u) || is_sra(u->myuser)) && (md = metadata_find(mu, METADATA_USER, "private:host:actual")))
		notice(usersvs.nick, origin, "Last login from: %s", md->value);
	else if (md = metadata_find(mu, METADATA_USER, "private:host:vhost"))
		notice(usersvs.nick, origin, "Last login from: %s", md->value);

	notice(usersvs.nick, origin, "Registered : %s (%s ago)", strfbuf, time_ago(mu->registered));

	if (!(mu->flags & MU_HIDEMAIL)
		|| (is_sra(u->myuser) || is_ircop(u) || u->myuser == mu))
		notice(usersvs.nick, origin, "Email      : %s%s", mu->email,
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
	if (MU_NOMEMO & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "NoMemo");
	}
	if (MU_EMAILMEMOS & mu->flags)
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "EMailMemos");
	}
	if (MU_ALIAS & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "Alias");
	}

	if (*buf)
		notice(usersvs.nick, origin, "Flags      : %s", buf);

	if ((mu->flags & MU_ALIAS) && (md = metadata_find(mu, METADATA_USER, "private:alias:parent")))
		notice(usersvs.nick, origin, "Parent     : %s", md->value);

	if (LIST_LENGTH(&mu->logins) == 0)
		notice(usersvs.nick, origin, "Logins from: <none>");
	else if (mu == u->myuser || is_ircop(u) || is_sra(u->myuser))
	{
		buf[0] = '\0';
		LIST_FOREACH(n, mu->logins.head)
		{
			if (strlen(buf) > 80)
			{
				notice(usersvs.nick, origin, "Logins from: %s", buf);
				buf[0] = '\0';
			}
			if (buf[0])
				strcat(buf, " ");
			strcat(buf, ((user_t *)(n->data))->nick);
		}
		if (buf[0])
			notice(usersvs.nick, origin, "Logins from: %s", buf);
	}
	else
		notice(usersvs.nick, origin, "Logins from: <hidden>");

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

                notice(usersvs.nick, origin, "%s was \2FROZEN\2 by %s on %s (%s)", mu->name, setter, strfbuf, reason);
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

		notice(usersvs.nick, origin, "%s was \2MARKED\2 by %s on %s (%s)", mu->name, setter, strfbuf, reason);
	}

	if ((MU_WAITAUTH & mu->flags) && (is_ircop(u) || is_sra(u->myuser)))
		notice(usersvs.nick, origin, "%s has not completed registration verification", mu->name);

	notice(usersvs.nick, origin, "*** \2End of Info\2 ***");

	logcommand(usersvs.me, u, CMDLOG_GET, "INFO %s", mu->name);
}
