/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 */

#include "atheme.h"
#include "hostserv.h"

DECLARE_MODULE_V1
(
	"hostserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void on_user_identify(user_t *u);

service_t *hs;
list_t hs_cmdtree;
list_t hs_helptree;
list_t conf_hs_table;

/* main services client routine */
static void hostserv(sourceinfo_t *si, int parc, char *parv[])
{
	char *cmd;
        char *text;
	char orig[BUFSIZE];

	/* this should never happen */
	if (parv[0][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* make a copy of the original for debugging */
	strlcpy(orig, parv[parc - 1], BUFSIZE);

	/* lets go through this to get the command */
	cmd = strtok(parv[parc - 1], " ");
	text = strtok(NULL, "");

	if (!cmd)
		return;
	if (*cmd == '\001')
	{
		handle_ctcp_common(si, cmd, text);
		return;
	}

	/* take the command through the hash table */
	command_exec_split(si->service, si, cmd, text, &hs_cmdtree);
}

void _modinit(module_t *m)
{
	hook_add_event("user_identify");
	hook_add_user_identify(on_user_identify);

	service_add("hostserv", hostserv, &hs_cmdtree, &conf_hs_table);
}

void _moddeinit(void)
{
	if (hs != NULL)
		service_delete(hostsvs.me);

	hook_del_user_identify(on_user_identify);
}

static void on_user_identify(user_t *u)
{
	myuser_t *mu = u->myuser;
	metadata_t *md;
	char buf[NICKLEN + 20];

	snprintf(buf, sizeof buf, "private:usercloak:%s", u->nick);
	md = metadata_find(mu, buf);
	if (md == NULL)
		md = metadata_find(mu, "private:usercloak");
	if (md == NULL)
		return;

	do_sethost(u, md->value);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
