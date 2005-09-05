/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ REGISTER function.
 *
 * $Id: link.c 2133 2005-09-05 01:19:23Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/link", FALSE, _modinit, _moddeinit,
	"$Id: link.c 2133 2005-09-05 01:19:23Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_link(char *origin);

command_t ns_link = { "LINK", "Links an unregistered nickname to your account.", AC_NONE, ns_cmd_link };

list_t *ns_cmdtree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	command_add(&ns_link, ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_link, ns_cmdtree);
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
		notice(nicksvs.nick, origin, "Syntax: LINK <target> [pass]");
		return;
	}

	if (!u->myuser)
	{
		notice(nicksvs.nick, origin, "You are not identified.");
		return;
	}

	if (user_find(nick))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is presently in use. "
			"For security reasons, you cannot link to nicknames "
			"that are presently in use.", nick);
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

	/* make sure it isn't registered already */
	mu = myuser_find(nick);
	if (mu && (!pass || (pass && (strcmp(mu->pass, pass)))))
	{
		/* should we reveal the e-mail address? (from ns_info.c) */
		if (!(mu->flags & MU_HIDEMAIL)
			|| (is_sra(u->myuser) || is_ircop(u) || u->myuser == mu))
			notice(nicksvs.nick, origin, "\2%s\2 is already registered to \2%s\2.", mu->name, mu->email);
		else
			notice(nicksvs.nick, origin, "\2%s\2 is already registered.", mu->name);

		return;
	}
	else if (mu != NULL && pass && !strcmp(mu->pass, pass))
		myuser_delete(mu->name);

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
		return;
	}

	snoop("LINK: %s to %s", nick, muptr->name);

	mu = myuser_add(nick, muptr->pass, muptr->email);
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;
	mu->flags |= config_options.defuflags;

	/* mark it as an alias */
	mu->flags |= MU_ALIAS;
	metadata_add(mu, METADATA_USER, "private:alias:parent", muptr->name);

	notice(nicksvs.nick, origin, "\2%s\2 is now linked to \2%s\2.", mu->name, muptr->name);
	hook_call_event("user_register", mu);
}
