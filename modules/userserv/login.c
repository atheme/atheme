/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGIN functions.
 *
 * $Id: login.c 4283 2005-12-29 02:34:51Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/login", FALSE, _modinit, _moddeinit,
	"$Id: login.c 4283 2005-12-29 02:34:51Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_login(char *origin);

command_t us_login = { "LOGIN", "Authenticates to a services account.",
			AC_NONE, us_cmd_login };

list_t *us_cmdtree, *us_helptree, *ms_cmdtree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	command_add(&us_login, us_cmdtree);
	help_addentry(us_helptree, "LOGIN", "help/userserv/login", NULL);
}

void _moddeinit()
{
	command_delete(&us_login, us_cmdtree);
	help_delentry(us_helptree, "LOGIN");
}

static void us_cmd_login(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	chanuser_t *cu;
	chanacs_t *ca;
	node_t *n, *tn;
	char *target = strtok(NULL, " ");
	char *password = strtok(NULL, " ");
	char buf[BUFSIZE], strfbuf[32];
	char lau[BUFSIZE], lao[BUFSIZE];
	struct tm tm;
	metadata_t *md_failnum;

	if (!target || !password)
	{
		notice(usersvs.nick, origin, "Insufficient parameters for \2LOGIN\2.");
		notice(usersvs.nick, origin, "Syntax: LOGIN <account> <password>");
		return;
	}

	mu = myuser_find(target);

	if (!mu)
	{
		notice(usersvs.nick, origin, "\2%s\2 is not a registered account.", target);
		return;
	}

        if (metadata_find(mu->name, METADATA_USER, "private:freeze:freezer"))
        {
		notice(usersvs.nick, origin, "You cannot login as \2%s\2 because the account has been frozen.", mu->name);
		logcommand(usersvs.me, u, CMDLOG_LOGIN, "failed LOGIN to %s (frozen)", mu->name);
                return;
        }

	if (u->myuser == mu)
	{
		notice(usersvs.nick, origin, "You are already logged in as \2%s\2.", mu->name);
		return;
	}

	/* we use this in both cases, so set it up here. may be NULL. */
	md_failnum = metadata_find(mu, METADATA_USER, "private:loginfail:failnum");

	if (verify_password(mu, password))
	{
		if (LIST_LENGTH(&mu->logins) >= me.maxlogins)
		{
			notice(usersvs.nick, origin, "There are already \2%d\2 sessions logged in to \2%s\2 (maximum allowed: %d).", LIST_LENGTH(&mu->logins), mu->name, me.maxlogins);
			logcommand(usersvs.me, u, CMDLOG_LOGIN, "failed LOGIN to %s (too many logins)", mu->name);
			return;
		}

		/* if they are identified to another account, nuke their session first */
		if (u->myuser)
		{
		        u->myuser->lastlogin = CURRTIME;
		        LIST_FOREACH_SAFE(n, tn, u->myuser->logins.head)
		        {
			        if (n->data == u)
		                {
		                        node_del(n, &u->myuser->logins);
		                        node_free(n);
		                        break;
		                }
		        }
		        u->myuser = NULL;
		}

		/*snoop("LOGIN:AS: \2%s\2 to \2%s\2", u->nick, mu->name);*/

		if (is_soper(mu))
		{
			snoop("SRA: \2%s\2 as \2%s\2", u->nick, mu->name);
			wallops("\2%s\2 is now an SRA.", u->nick);
		}

		myuser_notice(usersvs.nick, mu, "%s!%s@%s has just authenticated as you (%s)", u->nick, u->user, u->vhost, mu->name);

		u->myuser = mu;
		n = node_create();
		node_add(u, n, &mu->logins);

		/* keep track of login address for users */
		strlcpy(lau, u->user, BUFSIZE);
		strlcat(lau, "@", BUFSIZE);
		strlcat(lau, u->vhost, BUFSIZE);
		metadata_add(mu, METADATA_USER, "private:host:vhost", lau);

		/* and for opers.. */
		strlcpy(lao, u->user, BUFSIZE);
		strlcat(lao, "@", BUFSIZE);
		strlcat(lao, u->host, BUFSIZE);
		metadata_add(mu, METADATA_USER, "private:host:actual", lao);

		logcommand(usersvs.me, u, CMDLOG_LOGIN, "LOGIN");

		notice(usersvs.nick, origin, "You are now logged in as \2%s\2.", u->myuser->name);

		/* check for failed attempts and let them know */
		if (md_failnum && (atoi(md_failnum->value) > 0))
		{
			metadata_t *md_failtime, *md_failaddr;
			time_t ts;

			tm = *localtime(&mu->lastlogin);
			strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

			notice(usersvs.nick, origin, "\2%d\2 failed %s since %s.",
				atoi(md_failnum->value), (atoi(md_failnum->value) == 1) ? "login" : "logins", strfbuf);

			md_failtime = metadata_find(mu, METADATA_USER, "private:loginfail:lastfailtime");
			ts = atol(md_failtime->value);
			md_failaddr = metadata_find(mu, METADATA_USER, "private:loginfail:lastfailaddr");

			tm = *localtime(&ts);
			strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

			notice(usersvs.nick, origin, "Last failed attempt from: \2%s\2 on %s.",
				md_failaddr->value, strfbuf);

			metadata_delete(mu, METADATA_USER, "private:loginfail:failnum");	/* md_failnum now invalid */
			metadata_delete(mu, METADATA_USER, "private:loginfail:lastfailtime");
			metadata_delete(mu, METADATA_USER, "private:loginfail:lastfailaddr");
		}

		mu->lastlogin = CURRTIME;

		/* now we get to check for xOP */
		/* we don't check for host access yet (could match different
		 * entries because of services cloaks) */
		LIST_FOREACH(n, mu->chanacs.head)
		{
			ca = (chanacs_t *)n->data;

			cu = chanuser_find(ca->mychan->chan, u);
			if (cu)
			{
				if (ca->level & CA_AKICK)
				{
					ban(chansvs.nick, ca->mychan->name, u);
					kick(chansvs.nick, ca->mychan->name, u->nick, "User is banned from this channel");
					continue;
				}

				if (ircd->uses_owner && !(cu->modes & ircd->owner_mode) && should_owner(ca->mychan, ca->myuser))
				{
					cmode(chansvs.nick, ca->mychan->name, ircd->owner_mchar, CLIENT_NAME(u));
					cu->modes |= ircd->owner_mode;
				}

				if (ircd->uses_protect && !(cu->modes & ircd->protect_mode) && should_protect(ca->mychan, ca->myuser))
				{
					cmode(chansvs.nick, ca->mychan->name, ircd->protect_mchar, CLIENT_NAME(u));
					cu->modes |= ircd->protect_mode;
				}

				if (!(cu->modes & CMODE_OP) && ca->level & CA_AUTOOP)
				{
					cmode(chansvs.nick, ca->mychan->name, "+o", CLIENT_NAME(u));
					cu->modes |= CMODE_OP;
				}

				if (ircd->uses_halfops && !(cu->modes & (CMODE_OP | ircd->halfops_mode)) && ca->level & CA_AUTOHALFOP)
				{
					cmode(chansvs.nick, ca->mychan->name, "+h", CLIENT_NAME(u));
					cu->modes |= ircd->halfops_mode;
				}

				if (!(cu->modes & (CMODE_OP | ircd->halfops_mode | CMODE_VOICE)) && ca->level & CA_AUTOVOICE)
				{
					cmode(chansvs.nick, ca->mychan->name, "+v", CLIENT_NAME(u));
					cu->modes |= CMODE_VOICE;
				}

				if (ca->level & CA_USEDUPDATE)
					ca->mychan->used = CURRTIME;
			}
		}

		/* XXX: ircd_on_login supports hostmasking, we just dont have it yet. */
		/* don't allow them to join regonly chans until their
		 * email is verified
		 */
		if (!(mu->flags & MU_WAITAUTH))
			ircd_on_login(origin, mu->name, NULL);

		hook_call_event("user_identify", u);

		return;
	}

	snoop("LOGIN:AF: \2%s\2 to \2%s\2", u->nick, mu->name);
	logcommand(usersvs.me, u, CMDLOG_LOGIN, "failed LOGIN to %s (bad password)", mu->name);

	notice(usersvs.nick, origin, "Invalid password for \2%s\2.", mu->name);

	/* record the failed attempts */
	/* note that we reuse this buffer later when warning opers about failed logins */
	strlcpy(buf, u->nick, BUFSIZE);
	strlcat(buf, "!", BUFSIZE);
	strlcat(buf, u->user, BUFSIZE);
	strlcat(buf, "@", BUFSIZE);
	strlcat(buf, u->vhost, BUFSIZE);

	/* increment fail count */
	if (md_failnum && (atoi(md_failnum->value) > 0))
		md_failnum = metadata_add(mu, METADATA_USER, "private:loginfail:failnum",
								itoa(atoi(md_failnum->value) + 1));
	else
		md_failnum = metadata_add(mu, METADATA_USER, "private:loginfail:failnum", "1");
	metadata_add(mu, METADATA_USER, "private:loginfail:lastfailaddr", buf);
	metadata_add(mu, METADATA_USER, "private:loginfail:lastfailtime", itoa(CURRTIME));

	if (atoi(md_failnum->value) == 10)
	{
		time_t ts = CURRTIME;
		tm = *localtime(&ts);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		wallops("Warning: Numerous failed login attempts to \2%s\2. Last attempt received from \2%s\2 on %s.", mu->name, buf, strfbuf);
	}
}
