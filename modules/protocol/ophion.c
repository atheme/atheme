/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2008 Atheme Project (http://atheme.org/)
 * Copyright (C) 2020 Ophion LLC
 *
 * This file contains protocol support for ophion.
 */

#include <atheme.h>
#include <atheme/protocol/charybdis.h>

#define UF_NOLOGOUT UF_CUSTOM1
#define UF_HELPER   UF_CUSTOM2

static struct ircd Ophion = {
	.ircdname = "ophion",
	.tldprefix = "$$",
	.uses_uid = true,
	.uses_rcommand = false,
	.uses_owner = false,
	.uses_protect = false,
	.uses_halfops = false,
	.uses_p10 = false,
	.uses_vhost = false,
	.oper_only_modes = CMODE_EXLIMIT | CMODE_PERM | CMODE_IMMUNE,
	.owner_mode = CSTATUS_OWNER,
	.protect_mode = 0,
	.halfops_mode = 0,
	.owner_mchar = "+a",
	.protect_mchar = "+",
	.halfops_mchar = "+",
	.type = PROTOCOL_CHARYBDIS,
	.perm_mode = CMODE_PERM,
	.oimmune_mode = CMODE_IMMUNE,
	.ban_like_modes = "beIq",
	.except_mchar = 'e',
	.invex_mchar = 'I',
	.flags = IRCD_CIDR_BANS | IRCD_HOLDNICK | IRCD_TOPIC_NOCOLOUR,
	.capab_tokens = "IRCX",
};

static const struct cmode ophion_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'c', CMODE_NOCOLOR},
  { 'r', CMODE_REGONLY},
  { 'z', CMODE_OPMOD  },
  { 'g', CMODE_FINVITE},
  { 'L', CMODE_EXLIMIT},
  { 'P', CMODE_PERM   },
  { 'F', CMODE_FTARGET},
  { 'Q', CMODE_DISFWD },
  { 'M', CMODE_IMMUNE },
  { 'C', CMODE_NOCTCP },

  // following modes are added as extensions
  { 'N', CMODE_NPC	 },
  { 'S', CMODE_SSLONLY	 },
  { 'O', CMODE_OPERONLY  },
  { 'A', CMODE_ADMINONLY },
  { 'u', CMODE_NOFILTER  },

  { '\0', 0 }
};

static const struct cmode ophion_status_mode_list[] = {
  { 'a', CSTATUS_OWNER   },
  { 'o', CSTATUS_OP	 },
  { 'v', CSTATUS_VOICE	 },
  { '\0', 0 }
};

static const struct cmode ophion_prefix_mode_list[] = {
  { '.', CSTATUS_OWNER	 },
  { '@', CSTATUS_OP	 },
  { '+', CSTATUS_VOICE	 },
  { '\0', 0 }
};

static const struct cmode ophion_user_mode_list[] = {
  { 'p', UF_IMMUNE   },
  { 'a', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'D', UF_DEAF     },
  { 'S', UF_SERVICE  },
  { '\0', 0 }
};

static void
send_delete_user_memberships(struct user *u)
{
	sts(":%s TPROP %s %ld %ld member-of :",
		ME, CLIENT_NAME(u), u->ts, CURRTIME);
}

static void
burst_user_membership(struct user *u)
{
	/* if user is not logged in, then this is easy, just send a delete */
	if (u->myuser == NULL)
	{
		send_delete_user_memberships(u);
		return;
	}

	mowgli_list_t *memberships = privatedata_get(entity(u->myuser), "groupserv:membership");
	if (memberships == NULL)
	{
		send_delete_user_memberships(u);
		return;
	}

	char membership_buf[BUFSIZE] = "";
	mowgli_node_t *n;

	MOWGLI_LIST_FOREACH(n, memberships->head)
	{
		struct groupacs *ga = n->data;

		mowgli_strlcat(membership_buf, entity(ga->mg)->name, sizeof membership_buf);
		mowgli_strlcat(membership_buf, " ", sizeof membership_buf);
	}

	sts(":%s TPROP %s %ld %ld member-of :%s",
		ME, CLIENT_NAME(u), u->ts, CURRTIME,
		membership_buf);
}

static void
burst_myuser_membership(struct myuser *mu)
{
	return_if_fail(mu != NULL);

	mowgli_node_t *n;
	MOWGLI_LIST_FOREACH(n, mu->logins.head)
	{
		burst_user_membership(n->data);
	}
}

static void
burst_groupacs_change(struct groupacs *ga)
{
	return_if_fail(ga != NULL);

	if (!isuser(ga->mt))
		return;

	burst_myuser_membership(user(ga->mt));
}

static void
burst_metadata_change(struct hook_metadata_change *mdchange)
{
	return_if_fail(mdchange != NULL);

	/* don't let someone try to overwrite member-of property */
	if (!strcasecmp(mdchange->name, "member-of"))
		return;

	mowgli_node_t *n;
	MOWGLI_LIST_FOREACH(n, mdchange->target->logins.head)
	{
		struct user *u = n->data;

		sts(":%s TPROP %s %ld %ld %s :%s",
			ME, CLIENT_NAME(u), u->ts, CURRTIME,
			mdchange->name, mdchange->value);
	}
}

static void
persist_user_tprop(struct user *u, time_t target_ts, const char *key, const char *value)
{
	if (u->myuser == NULL)
	{
		slog(LG_DEBUG, "persist_user_tprop(): not persisting TPROP for anonymous user %s", u->nick);
		return;
	}

	if (target_ts != u->ts)
	{
		slog(LG_DEBUG, "persist_user_tprop(): invalid TPROP for %s", u->nick);
		return;
	}

	metadata_delete(u->myuser, key);
	metadata_add(u->myuser, key, value);
}

static void
persist_channel_tprop(struct mychan *mc, time_t target_ts, const char *key, const char *value)
{
	if (target_ts != mc->registered)
	{
		slog(LG_DEBUG, "persist_user_tprop(): invalid TPROP for %s", mc->name);
		return;
	}

	metadata_delete(mc, key);
	metadata_add(mc, key, value);
}

/*
 * :source PROP target key value
 *
 * parv[0] = target
 * parv[1] = key
 * parv[2] = value
 */
static void
m_prop(struct sourceinfo *si, int parc, char *parv[])
{
	if (*parv[0] == '#')
	{
		struct mychan *mc = mychan_find(parv[0]);

		if (mc == NULL)
		{
			slog(LG_DEBUG, "m_prop(): not tracking channel %s", parv[0]);
			return;
		}

		persist_channel_tprop(mc, mc->registered, parv[1], parv[2]);

		return;
	}

	struct user *u = user_find(parv[0]);
	if (u == NULL)
	{
		slog(LG_DEBUG, "m_prop(): unknown user %s", parv[0]);
		return;
	}

	persist_user_tprop(u, u->ts, parv[1], parv[2]);
}

/*
 * :source TPROP target targetTS propTS key :value
 *
 * parv[0] = target
 * parv[1] = targetTS
 * parv[2] = propTS
 * parv[3] = key
 * parv[4] = value
 */
static void
m_tprop(struct sourceinfo *si, int parc, char *parv[])
{
	if (*parv[0] == '#')
	{
		struct mychan *mc = mychan_find(parv[0]);

		if (mc == NULL)
		{
			slog(LG_DEBUG, "m_tprop(): not tracking channel %s", parv[0]);
			return;
		}

		persist_channel_tprop(mc, atol(parv[1]), parv[3], parv[4]);

		return;
	}

	struct user *u = user_find(parv[0]);
	if (u == NULL)
	{
		slog(LG_DEBUG, "m_tprop(): unknown user %s", parv[0]);
		return;
	}

	persist_user_tprop(u, atol(parv[1]), parv[3], parv[4]);
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/charybdis")

	mode_list = ophion_mode_list;
	status_mode_list = ophion_status_mode_list;
	prefix_mode_list = ophion_prefix_mode_list;
	user_mode_list = ophion_user_mode_list;

	ircd = &Ophion;

	hook_add_user_identify(burst_user_membership);
	hook_add_user_register(burst_myuser_membership);
	hook_add_groupacs_add(burst_groupacs_change);
	hook_add_groupacs_delete(burst_groupacs_change);
	hook_add_metadata_change(burst_metadata_change);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	hook_del_user_identify(burst_user_membership);
	hook_del_user_register(burst_myuser_membership);
	hook_del_groupacs_add(burst_groupacs_change);
	hook_del_groupacs_delete(burst_groupacs_change);
	hook_del_metadata_change(burst_metadata_change);
}

SIMPLE_DECLARE_MODULE_V1("protocol/ophion", MODULE_UNLOAD_CAPABILITY_NEVER)
