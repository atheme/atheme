/*
 * Copyright (c) 2005 Greg Feigenson
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Generates a new password, either n digits long (w/ userserv arg), or 7 digits
 *
 * $Id: us_generatepass.c 2721 2005-10-06 10:56:35Z kog $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/generatepass", FALSE, _modinit, _moddeinit,
	"$Id: us_generatepass.c 2721 2005-10-06 10:56:35Z kog $",
	"Epiphanic Networks <http://www.epiphanic.org>"
);

static void us_cmd_generatepass(char *origin);

command_t us_generatepass = { "GENERATEPASS", "Generates a random password.",
                        AC_NONE, us_cmd_generatepass };
                                                                                   
list_t *us_cmdtree;
list_t *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	command_add(&us_generatepass, us_cmdtree);
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	help_addentry(us_helptree, "GENERATEPASS", "help/userserv/generatepass", NULL);
	
	/* You'll need to create a helpfile and put in help/userserv */
}

void _moddeinit()
{
	command_delete(&us_generatepass, us_cmdtree);
	help_delentry(us_helptree, "GENERATEPASS");
}

static void us_cmd_generatepass(char *origin)
{
	int n;
	char *newpass = strtok(NULL, " ");
   
	if (newpass)
		n = atoi(newpass);
	
	if (!n)
		n = 7;

	newpass = gen_pw(n);

	notice(usersvs.nick, origin, "Randomly generated password: %s",newpass);
	free(newpass);
}
