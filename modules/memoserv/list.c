/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv LIST function
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/list", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_list(sourceinfo_t *si, int parc, char *parv[]);

command_t ms_list = { "LIST", N_(N_("Lists all of your memos.")),
                        AC_NONE, 0, ms_cmd_list, { .path = "memoserv/list" } };

void _modinit(module_t *m)
{
        service_named_bind_command("memoserv", &ms_list);
}

void _moddeinit()
{
	service_named_unbind_command("memoserv", &ms_list);
}

static void ms_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	/* Misc structs etc */
	mymemo_t *memo;
	mowgli_node_t *n;
	unsigned int i = 0;
	char strfbuf[32];
	struct tm tm;
	char line[512];
	char chan[CHANNELLEN];
	char *p;
	
	/* user logged in? */
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}
		
	
	command_success_nodata(si, ngettext(N_("You have %zu memo (%d new)."),
					    N_("You have %zu memos (%d new)."),
					    si->smu->memos.count), si->smu->memos.count, si->smu->memoct_new);
	
	/* Check to see if any memos */
	if (!si->smu->memos.count)
		return;

	/* Go to listing memos */
	command_success_nodata(si, " ");
	
	MOWGLI_ITER_FOREACH(n, si->smu->memos.head)
	{
		i++;
		memo = (mymemo_t *)n->data;
		tm = *localtime(&memo->sent);
		
		strftime(strfbuf, sizeof(strfbuf) - 1, 
			"%b %d %H:%M:%S %Y", &tm);
		
		snprintf(line, sizeof line, _("- %d From: %s Sent: %s"),
				i, memo->sender, strfbuf);
		if (memo->status & MEMO_CHANNEL && *memo->text == '#')
		{
			strlcat(line, " ", sizeof line);
			strlcat(line, _("To:"), sizeof line);
			strlcat(line, " ", sizeof line);
			strlcpy(chan, memo->text, sizeof chan);
			p = strchr(chan, ' ');
			if (p != NULL)
				*p = '\0';
			strlcat(line, chan, sizeof line);
		}
		if (!(memo->status & MEMO_READ))
		{
			strlcat(line, " ", sizeof line);
			strlcat(line, _("[unread]"), sizeof line);
		}
		command_success_nodata(si, "%s", line);
	}
	
	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
