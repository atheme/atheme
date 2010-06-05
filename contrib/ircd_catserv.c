/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented under doc/LICENSE.
 *
 * Meow!
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/ircd_catserv", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

service_t *catserv;
list_t catserv_cmdtree;
list_t catserv_conftable;

static void catserv_cmd_meow(sourceinfo_t *si, int parc, char *parv[]);
static void catserv_cmd_help(sourceinfo_t *si, int parc, char *parv[]);
static void catserv_handler(sourceinfo_t *si, int parc, char **parv);

command_t catserv_meow = { "MEOW", "Makes the cute little kitty-cat meow!",
				AC_NONE, 0, catserv_cmd_meow };
command_t catserv_help = { "HELP", "Displays contextual help information.",
				AC_NONE, 1, catserv_cmd_help };

void _modinit(module_t *m)
{
	catserv = service_add("catserv", catserv_handler, &catserv_cmdtree, &catserv_conftable);

	command_add(&catserv_meow, &catserv_cmdtree);
	command_add(&catserv_help, &catserv_cmdtree);
}

void _moddeinit()
{
	command_delete(&catserv_meow, &catserv_cmdtree);
	command_delete(&catserv_help, &catserv_cmdtree);

	service_delete(catserv);
}

static void catserv_cmd_meow(sourceinfo_t *si, int parc, char *parv[])
{
	command_success_nodata(si, "Meow!");
}

static void catserv_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	command_help(si, &catserv_cmdtree);
}

static void catserv_handler(sourceinfo_t *si, int parc, char *parv[])
{
        char orig[BUFSIZE];
	char *cmd;
	char *text;

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
        command_exec_split(catserv, si, cmd, text, &catserv_cmdtree);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
