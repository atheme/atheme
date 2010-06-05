/*
 * Copyright (c) 2007 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Rate limits account registrations.
 *
 */

#include "atheme.h"
#include <limits.h>

DECLARE_MODULE_V1
(
	"contrib/ns_ratelimitreg", false, _modinit, _moddeinit,
	"$Revision$",
	"Jilles Tjoelker <jilles -at- stack.nl>"
);

static void check_registration(hook_user_register_check_t *hdata);
static void handle_register(myuser_t *mu);

/* settings */
unsigned int ratelimitreg_max = 5; /* allow this many account registrations */
unsigned int ratelimitreg_period = 60; /* in this time */
unsigned int ratelimitreg_wallops_period = 3600; /* send wallops at most once an hour */

/* dynamic state */
unsigned int ratelimitreg_count = 0;
time_t ratelimitreg_firsttime = 0;

static list_t *conftable;

void _modinit(module_t *m)
{
	service_t *svs;

	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main");

	hook_add_event("user_can_register");
	hook_add_event("user_register");

	hook_add_user_can_register(check_registration);
	hook_add_user_register(handle_register);

	svs = service_find("nickserv");
	conftable = svs != NULL ? svs->conf_table : NULL;

	if (conftable != NULL)
	{
		add_uint_conf_item("RATELIMITREG_MAX", conftable, &ratelimitreg_max, 1, INT_MAX);
		add_duration_conf_item("RATELIMITREG_PERIOD", conftable, &ratelimitreg_period, "s");
		add_duration_conf_item("RATELIMITREG_WALLOPS_PERIOD", conftable, &ratelimitreg_wallops_period, "s");
	}
	else
		slog(LG_ERROR, "ns_ratelimitreg/_modinit(): no nickserv??");
}

void _moddeinit(void)
{
	hook_del_user_can_register(check_registration);
	hook_del_user_register(handle_register);

	if (conftable != NULL)
	{
		del_conf_item("RATELIMITREG_MAX", conftable);
		del_conf_item("RATELIMITREG_PERIOD", conftable);
		del_conf_item("RATELIMITREG_WALLOPS_PERIOD", conftable);
	}
}

static void check_registration(hook_user_register_check_t *hdata)
{
	static time_t lastwallops;

	if (hdata->approved)
		return;

	if ((unsigned int)(CURRTIME - ratelimitreg_firsttime) > ratelimitreg_period)
		ratelimitreg_count = 0, ratelimitreg_firsttime = CURRTIME;

	if (ratelimitreg_count > ratelimitreg_max &&
			!has_priv(hdata->si, PRIV_FLOOD))
	{
		command_fail(hdata->si, fault_toomany, "The system is currently too busy to process your registration, please try again later.");
		hdata->approved = 1;
		slog(LG_INFO, "REGISTER:THROTTLED: %s by \2%s\2", hdata->account,
				hdata->si->su != NULL ? hdata->si->su->nick : get_source_name(hdata->si));
		if ((unsigned int)(CURRTIME - lastwallops) >= ratelimitreg_wallops_period)
		{
			wallops("Registration of %s by %s was throttled.",
					hdata->account,
					get_oper_name(hdata->si));
			lastwallops = CURRTIME;
		}
	}
}

static void handle_register(myuser_t *mu)
{
	(void)mu;

	if ((unsigned int)(CURRTIME - ratelimitreg_firsttime) > ratelimitreg_period)
		ratelimitreg_count = 0, ratelimitreg_firsttime = CURRTIME;
	ratelimitreg_count++;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
