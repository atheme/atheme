/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService STATUS function.
 *
 * $Id: status.c 2011 2005-09-01 23:31:07Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1("nickserv/status", FALSE, _modinit, _moddeinit);

static void ns_cmd_status(char *origin);

command_t ns_status = { "STATUS", "Displays session information.", AC_NONE, ns_cmd_status };

list_t *ns_cmdtree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	command_add(&ns_status, ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_status, ns_cmdtree);
}

static void ns_cmd_status(char *origin)
{
	user_t *u = user_find(origin);

	if (!u->myuser)
	{
		notice(nicksvs.nick, origin, "You are not logged in.");
		return;
	}

	notice(nicksvs.nick, origin, "You are logged in as \2%s\2.", u->myuser->name);

	if (is_sra(u->myuser))
		notice(nicksvs.nick, origin, "You are a services root administrator.");

	if (is_admin(u))
		notice(nicksvs.nick, origin, "You are a server administrator.");

	if (is_ircop(u))
		notice(nicksvs.nick, origin, "You are an IRC operator.");
}
