/*
 * Copyright (c) 2005-2010 William Pitcock <nenolod@atheme.org>.
 * Copyright (c) 2006-2007 Jilles Tjoelker <jilles@stack.nl>.
 *
 * Rights to this code are documented in doc/LICENSE.
 */

#ifndef __GAMESERV_COMMON_H__
#define __GAMESERV_COMMON_H__

/*
 * Handle reporting for both fantasy commands and normal commands in GameServ
 * quickly and easily. Of course, sourceinfo has a vtable that can be manipulated,
 * but this is quicker and easier...                                  -- nenolod
 */
static inline void gs_command_report(sourceinfo_t *si, const char *fmt, ...)
{
	va_list args;
	char buf[BUFSIZE];

	va_start(args, fmt);
	vsnprintf(buf, BUFSIZE, fmt, args);
	va_end(args);

	if (si->c != NULL)
	{
		service_t *svs = service_find("gameserv");

		msg(svs->me->nick, si->c->name, "%s", buf);
	}
	else
		command_success_nodata(si, "%s", buf);
}

static inline bool gs_do_parameters(sourceinfo_t *si, int *parc, char ***parv, mychan_t **pmc)
{
	mychan_t *mc;
	chanuser_t *cu;
	metadata_t *md;
	const char *who;
	bool allow;

	if (*parc == 0)
		return true;
	if ((*parv)[0][0] == '#')
	{
		mc = mychan_find((*parv)[0]);
		if (mc == NULL)
		{
			command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), (*parv)[0]);
			return false;
		}
		if (mc->chan == NULL)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is currently empty."), mc->name);
			return false;
		}
		if (module_find_published("chanserv/set_gameserv"))
		{
			md = metadata_find(mc, "gameserv");
			if (md == NULL)
			{
				command_fail(si, fault_noprivs, _("%s is not enabled on \2%s\2."), "GAMESERV", mc->name);
				return false;
			}
			cu = chanuser_find(mc->chan, si->su);
			if (cu == NULL)
			{
				command_fail(si, fault_nosuch_target, _("You are not on \2%s\2."), mc->name);
				return false;
			}
			who = md->value;
			/* don't subvert +m; other modes can be subverted
			 * though
			 */
			if (mc->chan->modes & CMODE_MOD && !strcasecmp(who, "all"))
				who = "voice";
			if (!strcasecmp(who, "all"))
				allow = true;
			else if (!strcasecmp(who, "voice") || !strcmp(who, "1"))
				allow = cu->modes != 0 || chanacs_source_flags(mc, si) & (CA_AUTOOP | CA_OP | CA_AUTOVOICE | CA_VOICE);
			else if (!strcasecmp(who, "op"))
				allow = cu->modes & CSTATUS_OP || chanacs_source_flags(mc, si) & (CA_AUTOOP | CA_OP);
			else
			{
				command_fail(si, fault_noprivs, _("%s is not enabled on \2%s\2."), "GAMESERV", mc->name);
				return false;
			}
			if (!allow)
			{
				command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
				return false;
			}
		}
		(*parc)--;
		(*parv)++;
		*pmc = mc;
	}
	else
		*pmc = NULL;
	return true;
}

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
