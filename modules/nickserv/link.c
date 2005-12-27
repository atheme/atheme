/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LINK function.
 *
 * $Id: link.c 4219 2005-12-27 17:41:18Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/link", FALSE, _modinit, _moddeinit,
	"$Id: link.c 4219 2005-12-27 17:41:18Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_link(char *origin);

command_t ns_link = { "LINK", "Links an unregistered nickname to your account.", AC_NONE, ns_cmd_link };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	ns_helptree = module_locate_symbol("nickserv/main", "ns_helptree");
	command_add(&ns_link, ns_cmdtree);
	help_addentry(ns_helptree, "LINK", "help/nickserv/link", NULL);
}

void _moddeinit()
{
	command_delete(&ns_link, ns_cmdtree);
	help_delentry(ns_helptree, "LINK");
}

static void ns_cmd_link(char *origin)
{
	user_t *u = user_find(origin);
	metadata_t *md;
	myuser_t *mu, *tmu, *muptr;
	node_t *n;
	char *nick = strtok(NULL, " ");
	char *pass = strtok(NULL, " ");
	uint32_t i, tcnt;

	if (!nick)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters specified for \2LINK\2.");
		notice(nicksvs.nick, origin, "Syntax: LINK <target>");
		return;
	}

	if (!u->myuser)
	{
		notice(nicksvs.nick, origin, "You are not identified.");
		return;
	}

	if (u->myuser->flags & MU_WAITAUTH)
	{
		notice(nicksvs.nick, origin, "You must verify your registration before using the LINK command.");
		return;
	}

	if (user_find_named(nick))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is presently in use. "
			"For security reasons, you cannot link to nicknames "
			"that are presently in use.", nick);
		return;
	}

	/* see xmlrpc comments about sanity checking -- we need a better way to do this */
	if (IsDigit(*nick) || strchr(nick, '!') || (*nick == '-') || strchr(nick, ',')
		|| strchr(nick, '$') || strchr(nick, ':') || strchr(nick, '#')
		|| strchr(nick, '.') || !(strlen(nick) <= (NICKLEN - 1)))
	{
		notice(nicksvs.nick, origin, "Sorry, the nickname \2%s\2 cannot be registered.", nick);
		return;
	}

	if (!irccasecmp(u->myuser->name, nick))
	{
		notice(nicksvs.nick, origin, "You cannot link a nick to itself.");
		return;
	}

	if ((u->myuser->flags & MU_ALIAS) && (md = metadata_find(u->myuser,
		METADATA_USER, "private:alias:parent")))
	{
		notice(nicksvs.nick, origin, "You are trying to link to an alias. I am linking the nick to the parent nickname, \2%s\2 instead.",
			md->value);
		muptr = myuser_find(md->value);
	}
	else
		muptr = u->myuser;

	/* XXX this shouldn't happen, but it does */
	if (muptr->pass == NULL)
		return;

	/* make sure it isn't registered already */
	mu = myuser_find(nick);
	if (mu != NULL)
	{
		notice(nicksvs.nick, origin, "\2%s\2 is already registered.", mu->name);
		return;
	}

	/* make sure they're within limits */
	for (i = 0, tcnt = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			tmu = (myuser_t *)n->data;

			if (!strcasecmp(u->myuser->email, tmu->email))
				tcnt++;
		}
	}

	if (tcnt >= me.maxusers)
	{
		notice(nicksvs.nick, origin, "You have too many nicknames registered.", u->myuser->email);
		logcommand(nicksvs.me, u, CMDLOG_REGISTER, "failed LINK %s (too many for %s)", nick, u->myuser->email);
		return;
	}

	snoop("LINK: %s to %s", nick, muptr->name);

	/* if the original nickname had CRYPTPASS, pass it in so that
	 * myuser_add() knows that this password is already done
	 */
	mu = myuser_add(nick, muptr->pass, muptr->email,
		config_options.defuflags | (muptr->flags & MU_CRYPTPASS));
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;

	/* mark it as an alias */
	mu->flags |= MU_ALIAS;
	metadata_add(mu, METADATA_USER, "private:alias:parent", muptr->name);

	logcommand(nicksvs.me, u, CMDLOG_REGISTER, "LINK %s", nick);

	notice(nicksvs.nick, origin, "\2%s\2 is now linked to \2%s\2.", mu->name, muptr->name);
	hook_call_event("user_register", mu);
}
