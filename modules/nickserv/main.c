/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 6495 2006-09-26 15:57:09Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 6495 2006-09-26 15:57:09Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t ns_cmdtree;
list_t ns_helptree;

/* main services client routine */
static void nickserv(sourceinfo_t *si, int parc, char *parv[])
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
		handle_ctcp_common(cmd, text, si->su->nick, nicksvs.nick);
		return;
	}

	/* take the command through the hash table */
	command_exec_split(nicksvs.me, si, cmd, text, &ns_cmdtree);
}

struct
{
	char *nickstring, *accountstring;
} nick_account_trans[] =
{
	/* command descriptions */
	{ "Drops a nickname registration.", "Drops an account registration." },
	{ "Reclaims use of a nickname.", "Disconnects an old session." },
	{ "Prevents a nickname from expiring.", "Prevents an account from expiring." },
	{ "Registers a nickname.", "Registers an account." },
	{ "Lists nicknames registered matching a given pattern.", "Lists accounts matching a given pattern." },
	{ "Lists nicknames registered to an e-mail address.", "Lists accounts registered to an e-mail address." },
	{ "Freezes a nickname.", "Freezes an account." },
	{ "Resets a nickname password.", "Resets an account password." },
	{ "Returns a nickname to its owner.", "Returns a account to its owner." },
	{ "Verifies a nickname registration.", "Verifies an account registration." },

	/* messages */
	{ "Syntax: DROP <nickname> <password>", "Syntax: DROP <account> <password>" },
	{ "%s dropped the nickname \2%s\2", "%s dropped the account \2%s\2" },
	{ "The nickname \2%s\2 has been dropped.", "The account \2%s\2 has been dropped." },
	{ "Usage: FREEZE <nickname> <ON|OFF> [reason]", "Usage: FREEZE <account> <ON|OFF> [reason]" },
	{ "\2%s\2 is not a registered nickname.", "\2%s\2 is not a registered account." },
	{ "Usage: FREEZE <nickname> ON <reason>", "Usage: FREEZE <account> ON <reason>" },
	{ "The nickname \2%s\2 belongs to a services operator; it cannot be frozen.", "The account \2%s\2 belongs to a services operator; it cannot be frozen." },
	{ "%s froze the nickname \2%s\2 (%s).", "%s froze the account \2%s\2 (%s)." },
	{ "%s thawed the nickname \2%s\2.", "%s thawed the account \2%s\2." },
	{ "Usage: HOLD <nickname> <ON|OFF>", "Usage: HOLD <account> <ON|OFF>" },
	{ "%s set the HOLD option for the nickname \2%s\2.", "%s set the HOLD option for the account \2%s\2." },
	{ "%s removed the HOLD option on the nickname \2%s\2.", "%s removed the HOLD option on the account \2%s\2." },
	{ "Syntax: INFO <nickname>", "Syntax: INFO <account>" },
	{ "No nicknames matched pattern \2%s\2", "No accounts matched pattern \2%s\2" },
	{ "Nicknames matching e-mail address \2%s\2:", "Accounts matching e-mail address \2%s\2:" },
	{ "No nicknames matched e-mail address \2%s\2", "No accounts matched e-mail address \2%s\2" },
	{ "%s marked the nickname \2%s\2.", "%s marked the account \2%s\2." },
	{ "%s unmarked the nickname \2%s\2.", "%s unmarked the account \2%s\2." },
	{ "Syntax: REGISTER <password> <email>", "Syntax: REGISTER <account> <password> <email>" },
	{ "\2%s\2 has too many nicknames registered.", "\2%s\2 has too many accounts registered." },
	{ "An email containing nickname activation instructions has been sent to \2%s\2.", "An email containing account activation instructions has been sent to \2%s\2." },
	{ "If you do not complete registration within one day your nickname will expire.", "If you do not complete registration within one day your account will expire." },
	{ "%s registered the nick \2%s\2 and gained services operator privileges.", "%s registered the account \2%s\2 and gained services operator privileges." },
	{ "Syntax: RESETPASS <nickname>", "Syntax: RESETPASS <account>" },
	{ "Overriding MARK placed by %s on the nickname %s.", "Overriding MARK placed by %s on the account %s." },
	{ "The password for the nickname %s has been changed to %s.", "The password for the account %s has been changed to %s." },
	{ "This operation cannot be performed on %s, because the nickname has been marked by %s.", "This operation cannot be performed on %s, because the account has been marked by %s." },
	{ "The password for the nickname %s has been changed to %s.", "The password for the account %s has been changed to %s." },
	{ "%s reset the password for the nickname %s", "%s reset the password for the account %s" },
	{ "Usage: RETURN <nickname> <e-mail address>", "Usage: RETURN <account> <e-mail address>" },
	{ "%s returned the nickname \2%s\2 to \2%s\2", "%s returned the account \2%s\2 to \2%s\2" },
	{ "Syntax: SENDPASS <nickname>", "Syntax: SENDPASS <account>" },
	{ "Manipulates metadata entries associated with a nickname.", "Manipulates metadata entries associated with an account." },
	{ "You cannot use your nickname as a password.", "You cannot use your account name as a password." },
	{ "Changes the password associated with your nickname.", "Changes the password associated with your account." },
	{ "Syntax: TAXONOMY <nick>", "Syntax: TAXONOMY <account>" },
	{ "Syntax: VERIFY <operation> <nickname> <key>", "Syntax: VERIFY <operation> <account> <key>" },
	{ "Syntax: VHOST <nick> [vhost]", "Syntax: VHOST <account> [vhost]" },
	{ NULL, NULL }
};

static void nickserv_config_ready(void *unused)
{
	int i;

        if (nicksvs.me)
                del_service(nicksvs.me);

        nicksvs.me = add_service(nicksvs.nick, nicksvs.user,
                                 nicksvs.host, nicksvs.real, nickserv);
        nicksvs.disp = nicksvs.me->disp;

	if (usersvs.nick)
	{
		slog(LG_ERROR, "idiotic conf detected: nickserv enabled but userserv{} block present, ignoring userserv");
		free(usersvs.nick);
		usersvs.nick = NULL;
	}

	if (nicksvs.no_nick_ownership)
		for (i = 0; nick_account_trans[i].nickstring != NULL; i++)
			itranslation_create(nick_account_trans[i].nickstring,
					nick_account_trans[i].accountstring);
	else
		for (i = 0; nick_account_trans[i].nickstring != NULL; i++)
			itranslation_destroy(nick_account_trans[i].nickstring);

        hook_del_hook("config_ready", nickserv_config_ready);
}

void _modinit(module_t *m)
{
        hook_add_event("config_ready");
        hook_add_hook("config_ready", nickserv_config_ready);

        if (!cold_start)
        {
                nicksvs.me = add_service(nicksvs.nick, nicksvs.user,
			nicksvs.host, nicksvs.real, nickserv);
                nicksvs.disp = nicksvs.me->disp;
        }
	authservice_loaded++;
}

void _moddeinit(void)
{
        if (nicksvs.me)
	{
                del_service(nicksvs.me);
		nicksvs.me = NULL;
	}
	authservice_loaded--;
}
