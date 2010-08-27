/*
 * Copyright (c) 2006 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Rejects account registrations with certain email addresses.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/gen_regcheckemail", false, _modinit, _moddeinit,
	"$Revision: 7785 $",
	"Jilles Tjoelker <jilles -at- stack.nl>"
);

static void check_registration(hook_user_register_check_t *hdata);

void _modinit(module_t *m)
{
	hook_add_event("user_can_register");
	hook_add_user_can_register(check_registration);
}

void _moddeinit(void)
{
	hook_del_user_can_register(check_registration);
}

static void check_registration(hook_user_register_check_t *hdata)
{
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
			slog(LG_INFO, "REGISTER:BADEMAIL: %s to \2%s\2 by \2%s\2",
					hdata->account, hdata->email,
					hdata->si->su != NULL ? hdata->si->su->nick : get_source_name(hdata->si));
			return;
		}
		i++;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
