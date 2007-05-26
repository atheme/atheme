/*
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Rate limits account registrations.
 *
 * $Id: ns_ratelimitreg.c 8325 2007-05-26 03:42:03Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/ns_ratelimitreg", FALSE, _modinit, _moddeinit,
	"$Revision$",
	"Jilles Tjoelker <jilles -at- stack.nl>"
);

static void check_registration(void *vptr);
static void handle_register(void *vptr);

/* settings */
int ratelimitreg_max = 5; /* allow this many account registrations */
int ratelimitreg_period = 60; /* in this time */
int ratelimitreg_wallops_period = 3600; /* send wallops at most once an hour */

/* dynamic state */
int ratelimitreg_count = 0;
time_t ratelimitreg_firsttime = 0;

void _modinit(module_t *m)
{
	hook_add_event("user_can_register");
	hook_add_event("user_register");

	hook_add_hook("user_can_register", check_registration);
	hook_add_hook("user_register", handle_register);
}

void _moddeinit(void)
{
	hook_del_hook("user_can_register", check_registration);
	hook_del_hook("user_register", handle_register);
}

static void check_registration(void *vptr)
{
	hook_user_register_check_t *hdata = vptr;
	static time_t lastwallops;

	if (hdata->approved)
		return;

	if (ratelimitreg_firsttime + ratelimitreg_period > CURRTIME)
		ratelimitreg_count = 0, ratelimitreg_firsttime = CURRTIME;

	if (ratelimitreg_count > ratelimitreg_max &&
			!has_priv(hdata->si, PRIV_FLOOD))
	{
		command_fail(hdata->si, fault_toomany, "The system is currently too busy to process your registration, please try again later.");
		hdata->approved = 1;
		snoop("REGISTER:THROTTLED: %s by \2%s\2", hdata->account,
				hdata->si->su != NULL ? hdata->si->su->nick : get_source_name(hdata->si));
		if (lastwallops + ratelimitreg_wallops_period < CURRTIME)
		{
			wallops("Registration of %s by %s was throttled.",
					hdata->account,
					get_oper_name(hdata->si));
			lastwallops = CURRTIME;
		}
	}
}

static void handle_register(void *vptr)
{
	(void)vptr;

	if (ratelimitreg_firsttime + ratelimitreg_period > CURRTIME)
		ratelimitreg_count = 0, ratelimitreg_firsttime = CURRTIME;
	ratelimitreg_count++;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
