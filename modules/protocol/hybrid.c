/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for hybrid-based ircd.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/hybrid.h"

DECLARE_MODULE_V1("protocol/hybrid", true, _modinit, NULL, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Hybrid = {
	.ircdname = "Hybrid 7.1.2+",
	.tldprefix = "$$",
	.uses_uid = true,
	.uses_rcommand = false,
	.uses_owner = false,
	.uses_protect = false,
	.uses_halfops = false,
	.uses_p10 = false,
	.uses_vhost = false,
	.oper_only_modes = 0,
	.owner_mode = 0,
	.protect_mode = 0,
	.halfops_mode = 0,
	.owner_mchar = "+",
	.protect_mchar = "+",
	.halfops_mchar = "+",
	.type = PROTOCOL_RATBOX,
	.perm_mode = 0,
	.oimmune_mode = 0,
	.ban_like_modes = "beI",
	.except_mchar = 'e',
	.invex_mchar = 'I',
	.flags = IRCD_CIDR_BANS,
};

struct cmode_ hybrid_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { '\0', 0 }
};

struct extmode hybrid_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ hybrid_status_mode_list[] = {
  { 'o', CSTATUS_OP    },
  { 'v', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ hybrid_prefix_mode_list[] = {
  { '@', CSTATUS_OP    },
  { '+', CSTATUS_VOICE },
  { '\0', 0 }
};

struct cmode_ hybrid_user_mode_list[] = {
  { 'a', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'D', UF_DEAF     },
  { '\0', 0 }
};

/* *INDENT-ON* */
static void hybrid_quarantine_sts(user_t *source, user_t *victim, long duration, const char *reason)
{
	sts(":%s CAPTURE %s", CLIENT_NAME(source), CLIENT_NAME(victim));
}

static void m_tburst(sourceinfo_t *si, int parc, char *parv[])
{
	time_t chants = atol(parv[0]);
	channel_t *c = channel_find(parv[1]);
	time_t topicts = atol(parv[2]);
 
	if (c == NULL)
		return;

	/* Our uplink is trying to change the topic during burst,
	 * and we have already set a topic. Assume our change won.
	 * -- jilles */
	if (si->s != NULL && si->s->uplink == me.me &&
			!(si->s->flags & SF_EOB) && c->topic != NULL)
		return;

	if (c->ts < chants || (c->ts == chants && c->topicts >= topicts))
	{
		slog(LG_DEBUG, "m_tburst(): ignoring topic on %s", c->name);
		return;
	}

	handle_topic_from(si, c, parv[3], topicts, parv[4]);
}


void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/ts6-generic");

	mode_list = hybrid_mode_list;
	ignore_mode_list = hybrid_ignore_mode_list;
	status_mode_list = hybrid_status_mode_list;
	prefix_mode_list = hybrid_prefix_mode_list;
	user_mode_list = hybrid_user_mode_list;
	ignore_mode_list_size = ARRAY_SIZE(hybrid_ignore_mode_list);

	quarantine_sts = hybrid_quarantine_sts;

	ircd = &Hybrid;

	pcommand_add("TBURST", m_tburst, 5, MSRC_SERVER);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
