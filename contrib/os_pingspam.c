/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * ping spammer thingy
 *
 * $Id: os_pingspam.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/pingspam", FALSE, _modinit, _moddeinit,
	"$Id: os_pingspam.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

char *notices[] =
{
	"Scanning for proxies.",
	"Killing off bottlers.",
	"LOL ok so like we are teh SKANZ0RZING j00 becuz well like OMG deze bots r h3r3 an liek they are FL00DING!!#@! ignore plz",
	"gaben",
	"Please ignore this notice.",
	"Scanning for warez.",
	"All your pr0n are belong to us!",
	"Move over! This is the police!",
	"This notice brought to you by Burma-Shave.",
	"They're coming...",
	":)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(::)(:",
	"lolz!",
	"<Hikaru> your a pagan",
	"* Ads needs to shower soon",
	"<Hik`Coding> Don't make me get Yakuza on you",
	"beu fails it",
	"BAN KAI~!$"
};

char *phrases[] =
{
	"",
	" please-ignore",
	" proxy scan",
	" ignore me",
	" <3 neostats",
};

void pingspam(user_t *u);
static void user_add_hook(void *vptr);
static void os_cmd_pingspam(char *origin);
static void os_cmd_autopingspam(char *origin);

command_t os_pingspam = { "PINGSPAM", "Spam a user with pings from every service, plus some bonus notices.", PRIV_OMODE, os_cmd_pingspam };
command_t os_autopingspam = { "AUTOPINGSPAM", "Spam connecting users with pings from every service, plus some bonus notices (setting).", PRIV_ADMIN, os_cmd_autopingspam };

int spamming;
list_t *os_cmdtree;

void _modinit(module_t *m)
{
	spamming = 0;

	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	command_add(&os_pingspam, os_cmdtree);
	command_add(&os_autopingspam, os_cmdtree);

	hook_add_event("user_add");
	hook_add_hook("user_add", user_add_hook);
}

void _moddeinit()
{
	command_delete(&os_pingspam, os_cmdtree);
	command_delete(&os_autopingspam, os_cmdtree);
	hook_del_hook("user_add", user_add_hook);
}

static void user_add_hook(void *vptr)
{
	user_t *u = vptr;
	if(spamming)
		pingspam(u);
}

static void os_cmd_pingspam(char *origin)
{
	char *target = strtok(NULL, " ");
	user_t *u;

	if(!target)
	{
		notice(opersvs.nick, origin, "Usage: \2PINGSPAM\2 <target>");
		return;
	}

	if(!(u = user_find_named(target)))
	{
		notice(opersvs.nick, origin, "\2%s\2 is not on the network", target);
		return;
	}

	pingspam(u);
	notice(opersvs.nick, origin, "\2%s\2 has been pwned.", target);
	snoop("PINGSPAM: \2%s\2 -> \2%s\2", origin, target);
}

static void os_cmd_autopingspam(char *origin)
{
	char *mode = strtok(NULL, " ");

	if(!mode)
	{
		notice(opersvs.nick, origin, "Auto-pingspam is currently \2%s\2", spamming ? "ON" : "OFF");
		return;
	}

	if(strcasecmp(mode, "on") == 0 || atoi(mode))
	{
		spamming = 1;
		notice(opersvs.nick, origin, "Auto-pingspam is now \2ON\2");
	}else{
		spamming = 0;
		notice(opersvs.nick, origin, "Auto-pingspam is now \2OFF\2");
	}
}

void pingspam(user_t *u)
{
	user_t *sptr;
	node_t *n;
	int i;

	if(*globsvs.nick)
		for(i = 0;i < 6;i++)
			notice(globsvs.nick, u->nick, notices[rand() % sizeof(notices) / sizeof(char*)]);

	LIST_FOREACH(n, me.me->userlist.head)
	{
		sptr = n->data;
		msg(sptr->nick, u->nick, "\001PING %d%s\001",
				time(NULL),
				phrases[rand() % sizeof(phrases) / sizeof(char*)]
		   );
	}
}

