/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented under doc/LICENSE.
 *
 * Meow!
 *
 * $Id: ircd_catserv.c 4095 2005-12-15 08:55:00Z w00t $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/ircd_catserv", FALSE, _modinit, _moddeinit,
	"$Id: ircd_catserv.c 4095 2005-12-15 08:55:00Z w00t $",
	"Atheme Development Group <http://www.atheme.org>"
);

service_t *catserv;
list_t catserv_cmdtree;

static void catserv_cmd_meow(char *origin);
static void catserv_cmd_help(char *origin);
static void catserv_handler(char *origin, uint8_t parc, char **parv);

command_t catserv_meow = { "MEOW", "Makes the cute little kitty-cat meow!",
				AC_NONE, catserv_cmd_meow };
command_t catserv_help = { "HELP", "Displays contextual help information.",
				AC_NONE, catserv_cmd_help };

void _modinit(module_t *m)
{
	catserv = add_service("CatServ", "meow", "meowth.nu", "Kitty cat!", catserv_handler);

	command_add(&catserv_meow, &catserv_cmdtree);
	command_add(&catserv_help, &catserv_cmdtree);
}

void _moddeinit()
{
	command_delete(&catserv_meow, &catserv_cmdtree);
	command_delete(&catserv_help, &catserv_cmdtree);

	del_service(catserv);
}

static void catserv_cmd_meow(char *origin)
{
	notice(catserv->name, origin, "Meow!");
}

static void catserv_cmd_help(char *origin)
{
	command_help(catserv->name, origin, &catserv_cmdtree);
}

static void catserv_handler(char *origin, uint8_t parc, char *parv[])
{
        char *cmd;
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

        if (!cmd)
                return;

        /* take the command through the hash table */
        command_exec(catserv, origin, cmd, &catserv_cmdtree);
}
