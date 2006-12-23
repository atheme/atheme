/*
 * Copyright (c) 2006 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Rejects account registrations with certain email addresses.
 *
 * $Id: gen_regcheckemail.c 7383 2006-12-23 22:54:17Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/gen_regcheckemail", FALSE, _modinit, _moddeinit,
	"$Revision: 7383 $",
	"Jilles Tjoelker <jilles -at- stack.nl>"
);

static void check_registration(void *vptr);

void _modinit(module_t *m)
{
	hook_add_event("user_can_register");
	hook_add_hook("user_can_register", check_registration);
}

void _moddeinit(void)
{
	hook_del_hook("user_can_register", check_registration);
}

static void check_registration(void *vptr)
{
	hook_user_register_check_t *hdata = vptr;
	const char *bademails[] = { "*@hotmail.com", "*@msn.com",
		"*@gmail.com", "*@yahoo.com", NULL };
	int i;

	if (hdata->approved)
		return;

	i = 0;
	while (bademails[i] != NULL)
	{
		if (!match(bademails[i], hdata->email))
		{
			command_fail(hdata->si, fault_noprivs, "Sorry, we do not accept registrations with email addresses from that domain. Use another address.");
			hdata->approved = 1;
			return;
		}
		i++;
	}
}
