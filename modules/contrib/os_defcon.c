/*
 * Copyright (c) 2010 JD Horelick <jdhore1@gmail.com>
 * Rights to this code are as defined in doc/LICENSE.
 *
 * DEFCON implementation.
 *
 * By default, any setting except 5 will expire after 15 minutes,
 * to change this, add a defcon_timeout = X; option to the operserv{}
 * block in your atheme.con. X = amount of time in minutes for a defcon
 * setting to time out/expire.
 *
 */
#include "atheme.h"
#define DEFCON_CMODE "R"

DECLARE_MODULE_V1
(
	"operserv/defcon", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_defcon(sourceinfo_t *si, int parc, char *parv[]);
static void defcon_nouserreg(hook_user_register_check_t *hdata);
static void defcon_nochanreg(hook_channel_register_check_t *hdatac);
static void defcon_useradd(hook_user_nick_t *data);
static void defcon_timeoutfunc(void *dummy);
static int level = 5;
static unsigned int defcon_timeout = 900;

command_t os_defcon = { "DEFCON", N_("Implements Defense Condition lockdowns."), PRIV_ADMIN, 1, os_cmd_defcon, { .path = "contrib/defcon" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_defcon);
	TAINT_ON("Using os_defcon", "Use of os_defcon is unsupported and not recommend. Use only at your own risk.");

	/* Hooks for all the stuff defcon disables */
	hook_add_event("user_can_register");
	hook_add_user_can_register(defcon_nouserreg);
	hook_add_event("channel_can_register");
	hook_add_channel_can_register(defcon_nochanreg);
	hook_add_event("user_add");
	hook_add_user_add(defcon_useradd);

	service_t *svs;
	svs = service_find("operserv");
	add_duration_conf_item("DEFCON_TIMEOUT", svs->conf_table, 0, &defcon_timeout, "m", 900);
}

void _moddeinit(void)
{
	service_named_unbind_command("operserv", &os_defcon);

	hook_del_user_can_register(defcon_nouserreg);
	hook_del_channel_can_register(defcon_nochanreg);
	hook_del_user_add(defcon_useradd);

	service_t *svs;
	svs = service_find("operserv");
	del_conf_item("DEFCON_TIMEOUT", svs->conf_table);

	event_delete(defcon_timeoutfunc, NULL);
}

static void defcon_nouserreg(hook_user_register_check_t *hdata)
{
	return_if_fail(hdata != NULL);
	return_if_fail(hdata->si != NULL);

	if (level < 5)
	{
		command_fail(hdata->si, fault_badparams, _("Registrations are currently disabled on this network, please try again later."));
		hdata->approved++;
	}
}

static void defcon_nochanreg(hook_channel_register_check_t *hdatac)
{
	return_if_fail(hdatac != NULL);
	return_if_fail(hdatac->si != NULL);

	if (level < 5)
	{
		command_fail(hdatac->si, fault_badparams, _("Registrations are currently disabled on this network, please try again later."));
		hdatac->approved++;
	}
}

static void defcon_useradd(hook_user_nick_t *data)
{
	user_t *u = data->u;

	if (!u)
		return;

	if (is_internal_client(u))
		return;

	if (level == 1)
	{
		slog(LG_INFO, "DEFCON:KLINE: %s!%s@%s", u->nick, u->user, u->host);
		kline_sts("*", u->user, u->host, 900, "This network is currently not accepting connections, please try again later.");
	}
}

static void defcon_svsignore(void)
{
	svsignore_t *svsignore;
	mowgli_node_t *n, *tn;

	if (level <= 2)
	{
		MOWGLI_ITER_FOREACH(n, svs_ignore_list.head)
		{
			svsignore = (svsignore_t *)n->data;

			if (!strcasecmp(svsignore->mask, "*@*"))
				return;
		}

		slog(LG_INFO, "DEFCON:IGNORE:ADD");
		svsignore = svsignore_add("*@*", "DEFCON Level 1 or 2 activated");
		svsignore->setby = "DEFCON";
		svsignore->settime = CURRTIME;
	}
	else if (level >= 3)
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, svs_ignore_list.head)
		{
			svsignore = (svsignore_t *)n->data;

			if (!strcasecmp(svsignore->mask,"*@*"))
			{
				slog(LG_INFO, "DEFCON:IGNORE:REMOVE");
				svsignore_delete(svsignore);
			}
		}
	}
}

static void defcon_forcechanmodes(void)
{
	channel_t *chptr;
	mowgli_patricia_iteration_state_t state;
	service_t *svs;
	char modesetbuf[256];
	char modeunsetbuf[256];

	svs = service_find("operserv");

	if (level <= 3)
	{
		snprintf(modesetbuf, sizeof modesetbuf, "+%s", DEFCON_CMODE);
		slog(LG_INFO, "DEFCON:MODE: %s", modesetbuf);
		MOWGLI_PATRICIA_FOREACH(chptr, &state, chanlist)
		{
			channel_mode_va(svs->me, chptr, 1, modesetbuf);
		}
	}
	else if (level >= 4)
	{
		snprintf(modeunsetbuf, sizeof modeunsetbuf, "-%s", DEFCON_CMODE);
		slog(LG_INFO, "DEFCON:MODE: %s", modeunsetbuf);
		MOWGLI_PATRICIA_FOREACH(chptr, &state, chanlist)
		{
			channel_mode_va(svs->me, chptr, 1, modeunsetbuf);
		}
	}
}

static void defcon_timeoutfunc(void *dummy)
{
	service_t *svs;
	char buf[BUFSIZE];

	svs = service_find("operserv");

	level = 5;
	defcon_svsignore();
	defcon_forcechanmodes();
	slog(LG_INFO, "DEFCON:TIMEOUT");

	snprintf(buf, sizeof buf, "The DEFCON level is now back to normal (\2%d\2). Sorry for any inconvenience this caused.", level);
	notice_global_sts(svs->me, "*", buf);
}

static void os_cmd_defcon(sourceinfo_t *si, int parc, char *parv[])
{
	char *defcon = parv[0];
	char buf[BUFSIZE];

	if (!defcon)
	{
		command_success_nodata(si, _("Defense condition is currently level \2%d\2."), level);
		return;
	}

	level = atoi(defcon);

	if ((level <= 0) || (level > 5))
	{
		command_fail(si, fault_badparams, _("Defcon level must be between 1 and 5"));
		level = 5;
		return;
	}

	/* Call the 2 functions that don't use hooks */
	defcon_svsignore();
	defcon_forcechanmodes();

	if (level < 5)
	{
		snprintf(buf, sizeof buf, "The DEFCON level has been changed to \2%d\2. We apologize for any inconvenience.", level);

		int i = event_find(defcon_timeoutfunc, NULL);

		if (i == -1)
			event_add_once("defcon_timeout", defcon_timeoutfunc, NULL, defcon_timeout);
	}
	else
	{
		snprintf(buf, sizeof buf, "The DEFCON level is now back to normal (\2%d\2). Sorry for any inconvenience this caused.", level);
		event_delete(defcon_timeoutfunc, NULL);
	}

	notice_global_sts(si->service->me, "*", buf);
	command_success_nodata(si, _("Defense condition set to level \2%d\2."), level);
	wallops(_("\2%s\2 set Defense condition to level \2%d\2."), get_oper_name(si), level);
	logcommand(si, CMDLOG_ADMIN, "DEFCON: \2%d\2", level);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
