/*
 * Copyright (c) 2010 William Pitcock, et al.
 * The rights to this code are as documented under doc/LICENSE.
 *
 * Automatically AKILL a list of clients, given their operating parameters.
 *
 * Basically this builds a keyword patricia.  O(NICKLEN) lookups at the price
 * of a longer startup process.
 *
 * Configuration.
 * ==============
 *
 * This module adds a new block to the config file:
 *
 *     nicklists {
 *         all = "/home/nenolod/atheme-production.hg4576/etc/nicklists/azn.txt";
 *         nick = "/home/nenolod/atheme-production.hg4576/etc/nicklists/bottler-nicks.txt";
 *         user = "/home/nenolod/atheme-production.hg4576/etc/nicklists/bottler-users.txt";
 *         real = "/home/nenolod/atheme-production.hg4576/etc/nicklists/bottler-gecos.txt";
 *     };
 *
 * You can add multiple all, nick, user and real entries.  The entries will be merged.
 * I would also like to say: fuck you GNAA, you guys need to go play in fucking traffic.
 * Thanks for reading my crappy docs, and have a nice day.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/os_akillnicklist", false, _modinit, _moddeinit,
	"0.1",
	"Atheme Development Group <http://www.atheme.org>"
);

static mowgli_patricia_t *akillalllist = NULL;
static mowgli_patricia_t *akillnicklist = NULL;
static mowgli_patricia_t *akilluserlist = NULL;
static mowgli_patricia_t *akillreallist = NULL;

static mowgli_list_t conft = { NULL, NULL, 0 };

static void
add_contents_of_file_to_list(const char *filename, mowgli_patricia_t *list)
{
	char value[BUFSIZE];
	FILE *f;

	f = fopen(filename, "r");
	if (!f)
		return;

	while (fgets(value, BUFSIZE, f) != NULL)
	{
		strip(value);

		if (!*value)
			continue;

		mowgli_patricia_add(list, value, (void *) 0x1);
	}

	fclose(f);
}

static int
nicklist_config_handler_all(config_entry_t *entry)
{
	add_contents_of_file_to_list(entry->ce_vardata, akillalllist);

	return 0;
}

static int
nicklist_config_handler_nick(config_entry_t *entry)
{
	add_contents_of_file_to_list(entry->ce_vardata, akillnicklist);

	return 0;
}

static int
nicklist_config_handler_user(config_entry_t *entry)
{
	add_contents_of_file_to_list(entry->ce_vardata, akilluserlist);

	return 0;
}

static int
nicklist_config_handler_real(config_entry_t *entry)
{
	add_contents_of_file_to_list(entry->ce_vardata, akillreallist);

	return 0;
}

static void
aknl_nickhook(hook_user_nick_t *data)
{
	user_t *u;
	bool doit = false;
	char *username;

	return_if_fail(data != NULL);
	return_if_fail(data->u != NULL);

	u = data->u;

	if (is_internal_client(u))
		return;

	if (is_autokline_exempt(u))
		return;

	username = u->user;
	if (*username == '~')
		username++;

	if (mowgli_patricia_retrieve(akillalllist, u->nick) != NULL && 
	    mowgli_patricia_retrieve(akillalllist, username) != NULL &&
	    mowgli_patricia_retrieve(akillalllist, u->gecos) != NULL)
		doit = true;

	if (mowgli_patricia_retrieve(akillnicklist, u->nick) != NULL)
		doit = true;

	if (mowgli_patricia_retrieve(akilluserlist, username) != NULL)
		doit = true;

	if (mowgli_patricia_retrieve(akillreallist, u->gecos) != NULL)
		doit = true;

	if (!doit)
		return;

	slog(LG_INFO, "AKNL: k-lining \2%s\2!%s@%s [%s] due to appearing to be a possible spambot", u->nick, u->user, u->host, u->gecos);
	kline_sts("*", "*", u->host, 86400, "Possible spambot");
}

void
_modinit(module_t *m)
{
	add_subblock_top_conf("NICKLISTS", &conft);
	add_conf_item("ALL", &conft, nicklist_config_handler_all);
	add_conf_item("NICK", &conft, nicklist_config_handler_nick);
	add_conf_item("USER", &conft, nicklist_config_handler_user);
	add_conf_item("REAL", &conft, nicklist_config_handler_real);

	akillalllist = mowgli_patricia_create(strcasecanon);
	akillnicklist = mowgli_patricia_create(strcasecanon);
	akilluserlist = mowgli_patricia_create(strcasecanon);
	akillreallist = mowgli_patricia_create(strcasecanon);

	hook_add_event("user_add");
	hook_add_user_add(aknl_nickhook);
	hook_add_event("user_nickchange");
	hook_add_user_nickchange(aknl_nickhook);
}

void
_moddeinit(void)
{
	hook_del_user_add(aknl_nickhook);
	hook_del_user_nickchange(aknl_nickhook);

	del_conf_item("ALL", &conft);
	del_conf_item("NICK", &conft);
	del_conf_item("USER", &conft);
	del_conf_item("REAL", &conft);
	del_top_conf("NICKLISTS");
}
