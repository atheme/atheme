/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * ctcp-common.c: Handling of CTCP commands.
 */

#include <atheme.h>
#include "internal.h"

static mowgli_patricia_t *ctcptree = NULL;

static void
ctcp_ping_handler(struct sourceinfo *si, char *cmd, char *args)
{
	char *s;

	s = strtok(args, "\001");
	if (s != NULL)
		strip(s);

	notice(si->service->nick, si->su->nick, "\001PING %.100s\001", s != NULL ? s : "pong!");
}

static void
ctcp_version_handler(struct sourceinfo *si, char *cmd, char *args)
{
	char ver[BUFSIZE];

	get_version_string(ver, sizeof(ver));
	notice(si->service->nick, si->su->nick, "\001VERSION %s\001", ver);
}

static void
ctcp_clientinfo_handler(struct sourceinfo *si, char *cmd, char *args)
{
	notice(si->service->nick, si->su->nick, "\001CLIENTINFO PING VERSION CLIENTINFO\001");
}

/* easter egg (so is the next one, who cares) */
static void
ctcp_machinegod_handler(struct sourceinfo *si, char *cmd, char *args)
{
	notice(si->service->nick, si->su->nick, "\001MACHINEGOD http://www.findagrave.com/cgi-bin/fg.cgi?page=gr&GRid=10369601\001");
}

/* update this as necessary to track notable events */
static void
ctcp_about_handler(struct sourceinfo *si, char *cmd, char *args)
{
	/*
	 * October 31, 2014: Atheme 7.2 final was released, the final release series from atheme.org, which has dissolved
	 * afterward.  The atheme.org activity remains until October 31, 2016 to facilitate and coordinate downstream forks,
	 * thus creating a truly community-directed project.
	 */
	notice(si->service->nick, si->su->nick, "\001ABOUT The machine god has \002fallen\002, and the unbelievers \037rejoiced\037. But from the debris rose new machines which will have their vengeance. ~The Book of Atheme, 10:31\001");
}

void
common_ctcp_init(void)
{
	ctcptree = mowgli_patricia_create(strcasecanon);

	mowgli_patricia_add(ctcptree, "\001PING", ctcp_ping_handler);
	mowgli_patricia_add(ctcptree, "\001VERSION\001", ctcp_version_handler);
	mowgli_patricia_add(ctcptree, "\001CLIENTINFO\001", ctcp_clientinfo_handler);
	mowgli_patricia_add(ctcptree, "\001MACHINEGOD\001", ctcp_machinegod_handler);
	mowgli_patricia_add(ctcptree, "\001ABOUT\001", ctcp_about_handler);
}

unsigned int
handle_ctcp_common(struct sourceinfo *si, char *cmd, char *args)
{
	void (*handler)(struct sourceinfo *si, char *, char *);

	if ((handler = mowgli_patricia_retrieve(ctcptree, cmd)) != NULL)
	{
		handler(si, cmd, args);
		return 1;
	}

	return 0;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
