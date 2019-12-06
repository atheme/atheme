/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2007 Atheme Project (http://atheme.org/)
 *
 * Changes and shows nickname access lists.
 */

#include <atheme.h>

static bool
username_is_random(const char *name)
{
	const char *p;
	int lower = 0, upper = 0, digit = 0;

	if (*name == '~')
		name++;
	if (strlen(name) < 9)
		return false;
	p = name;
	while (*p != '\0')
	{
		if (isdigit((unsigned char)*p))
			digit++;
		else if (isupper((unsigned char)*p))
			upper++;
		else if (islower((unsigned char)*p))
			lower++;
		p++;
	}
	if (digit >= 4 && lower + upper > 1)
		return true;
	if (lower == 0 || upper == 0 || (upper <= 2 && isupper(*name)))
		return false;
	return true;
}

static char *
construct_mask(struct user *u)
{
	static char mask[USERLEN + 1 + HOSTLEN + 1];
	const char *dynhosts[] = { "*dyn*.*", "*dial*.*.*", "*dhcp*.*.*",
		"*.t-online.??", "*.t-online.???",
		"*.t-dialin.??", "*.t-dialin.???",
		"*.t-ipconnect.??", "*.t-ipconnect.???",
		"*.ipt.aol.com", NULL };
	int i;
	bool hostisdyn = false, havedigits;
	const char *p, *prevdot, *lastdot;

	for (i = 0; dynhosts[i] != NULL; i++)
		if (!match(dynhosts[i], u->host))
			hostisdyn = true;
	if (hostisdyn)
	{
		// note that all dyn patterns contain a dot
		p = u->host;
		prevdot = u->host;
		lastdot = strrchr(u->host, '.');
		havedigits = true;
		while (*p)
		{
			if (*p == '.')
			{
				if (!havedigits || p == lastdot || !strcasecmp(p, ".Level3.net"))
					break;
				prevdot = p;
				havedigits = false;
			}
			else if (isdigit((unsigned char)*p))
				havedigits = true;
			p++;
		}
		snprintf(mask, sizeof mask, "%s@*%s", u->user, prevdot);
	}
	else if (username_is_random(u->user))
		snprintf(mask, sizeof mask, "*@%s", u->host);
	else if (u->ip != NULL && !strcmp(u->host, u->ip) &&
			(p = strrchr(u->ip, '.')) != NULL)
		snprintf(mask, sizeof mask, "%s@%.*s.0/24", u->user, (int)(p - u->ip), u->ip);
	else
		snprintf(mask, sizeof mask, "%s@%s", u->user, u->host);
	return mask;
}

static bool
mangle_wildcard_to_cidr(const char *host, char *dest, size_t destlen)
{
	int i;
	const char *p;

	p = host;

	if ((p[0] != '0' || p[1] != '.') && ((i = atoi(p)) < 1 || i > 255))
		return false;
	while (isdigit((unsigned char)*p))
		p++;
	if (*p++ != '.')
		return false;
	if (p[0] == '*' && p[1] == '\0')
	{
		snprintf(dest, destlen, "%.*s0.0.0/8", (int)(p - host), host);
		return true;
	}

	if ((p[0] != '0' || p[1] != '.') && ((i = atoi(p)) < 1 || i > 255))
		return false;
	while (isdigit((unsigned char)*p))
		p++;
	if (*p++ != '.')
		return false;
	if (p[0] == '*' && (p[1] == '\0' || (p[1] == '.' && p[2] == '*' && p[3] == '\0')))
	{
		snprintf(dest, destlen, "%.*s0.0/16", (int)(p - host), host);
		return true;
	}

	if ((p[0] != '0' || p[1] != '.') && ((i = atoi(p)) < 1 || i > 255))
		return false;
	while (isdigit((unsigned char)*p))
		p++;
	if (*p++ != '.')
		return false;
	if (p[0] == '*' && p[1] == '\0')
	{
		snprintf(dest, destlen, "%.*s0/24", (int)(p - host), host);
		return true;
	}

	return false;
}

static void
myuser_access_delete_enforce(struct myuser *mu, char *mask)
{
	mowgli_list_t l = {NULL, NULL, 0};
	mowgli_node_t *n, *tn;
	struct mynick *mn;
	struct user *u;
	struct hook_nick_enforce hdata;

	// find users who get access via the access list
	MOWGLI_ITER_FOREACH(n, mu->nicks.head)
	{
		mn = n->data;
		u = user_find_named(mn->nick);
		if (u != NULL && u->myuser != mu && myuser_access_verify(u, mu))
			mowgli_node_add(u, mowgli_node_create(), &l);
	}

	// remove mask
	myuser_access_delete(mu, mask);

	// check if those users still have access
	MOWGLI_ITER_FOREACH_SAFE(n, tn, l.head)
	{
		u = n->data;
		mowgli_node_delete(n, &l);
		mowgli_node_free(n);
		if (!myuser_access_verify(u, mu))
		{
			mn = mynick_find(u->nick);
			if (mn != NULL)
			{
				hdata.u = u;
				hdata.mn = mn;
				hook_call_nick_enforce(&hdata);
			}
		}
	}
}

static void
ns_cmd_access(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	mowgli_node_t *n;
	char *mask;
	char *host;
	char *p;
	char mangledmask[NICKLEN + 1 + HOSTLEN + 1 + 10];

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS");
		command_fail(si, fault_needmoreparams, _("Syntax: ACCESS ADD|DEL|LIST [mask]"));
		return;
	}

	if (!strcasecmp(parv[0], "LIST"))
	{
		if (parc < 2)
		{
			mu = si->smu;
			if (mu == NULL)
			{
				command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
				return;
			}
		}
		else
		{
			if (!has_priv(si, PRIV_USER_AUSPEX))
			{
				command_fail(si, fault_noprivs, _("You are not authorized to use the target argument."));
				return;
			}

			if (!(mu = myuser_find_ext(parv[1])))
			{
				command_fail(si, fault_badparams, STR_IS_NOT_REGISTERED, parv[1]);
				return;
			}
		}

		if (mu != si->smu)
			logcommand(si, CMDLOG_ADMIN, "ACCESS:LIST: \2%s\2", entity(mu)->name);
		else
			logcommand(si, CMDLOG_GET, "ACCESS:LIST");

		command_success_nodata(si, _("Access list for \2%s\2:"), entity(mu)->name);

		MOWGLI_ITER_FOREACH(n, mu->access_list.head)
		{
			mask = n->data;
			command_success_nodata(si, "- %s", mask);
		}

		command_success_nodata(si, _("End of \2%s\2 access list."), entity(mu)->name);
	}
	else if (!strcasecmp(parv[0], "ADD"))
	{
		mu = si->smu;
		if (parc < 2)
		{
			if (si->su == NULL)
			{
				command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS ADD");
				command_fail(si, fault_needmoreparams, _("Syntax: ACCESS ADD <mask>"));
				return;
			}
			else
				mask = construct_mask(si->su);
		}
		else
			mask = parv[1];
		if (mu == NULL)
		{
			command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
			return;
		}
		if (mask[0] == '*' && mask[1] == '!')
			mask += 2;
		if (strlen(mask) > USERLEN + 1 + HOSTLEN)
		{
			command_fail(si, fault_badparams, _("Invalid mask \2%s\2."), parv[1]);
			return;
		}
		p = mask;
		while (*p != '\0')
		{
			if (!isprint((unsigned char)*p) || *p == ' ' || *p == '!')
			{
				command_fail(si, fault_badparams, _("Invalid mask \2%s\2."), parv[1]);
				return;
			}
			p++;
		}
		host = strchr(mask, '@');
		if (host == NULL) // account name access masks?
		{
			command_fail(si, fault_badparams, _("Invalid mask \2%s\2."), parv[1]);
			return;
		}
		host++;

		// try mangling to cidr
		mowgli_strlcpy(mangledmask, mask, sizeof mangledmask);
		if (mangle_wildcard_to_cidr(host, mangledmask + (host - mask), sizeof mangledmask - (host - mask)))
		{
			host = mangledmask + (host - mask);
			mask = mangledmask;
		}

		// more checks
		if (si->su != NULL && (!strcasecmp(host, si->su->host) || !strcasecmp(host, si->su->vhost)))
			; // it's their host, allow it
		else if (host[0] == '.' || host[0] == ':' || host[0] == '\0' || host[1] == '\0' || host == mask + 1 || strchr(host, '@') || strstr(host, ".."))
		{
			command_fail(si, fault_badparams, _("Invalid mask \2%s\2."), parv[1]);
			return;
		}
		else if ((strchr(host, '*') || strchr(host, '?')) && (mask[0] == '*' && mask[1] == '@'))
		{
			// can't use * username and wildcarded host
			command_fail(si, fault_badparams, _("Too wide mask \2%s\2."), parv[1]);
			return;
		}
		else if ((p = strrchr(host, '/')) != NULL)
		{
			if (isdigit((unsigned char)p[1]) && (atoi(p + 1) < 16 || (mask[0] == '*' && mask[1] == '@')))
			{
				command_fail(si, fault_badparams, _("Too wide mask \2%s\2."), parv[1]);
				return;
			}
			if (host[0] == '*')
			{
				command_fail(si, fault_badparams, _("Too wide mask \2%s\2."), parv[1]);
				return;
			}
		}
		else
		{
			if (strchr(host, ':'))
			{
				// No wildcarded IPs
				if (strchr(host, '?') || strchr(host, '*'))
				{
					command_fail(si, fault_badparams, _("Too wide mask \2%s\2."), parv[1]);
					return;
				}
			}
			else
			{
				p = strrchr(host, '.');
				if (p == NULL)
					p = host;

				// No wildcarded IPs
				if (isdigit((unsigned char)p[1]) && (strchr(host, '*') || strchr(host, '?')))
				{
					command_fail(si, fault_badparams, _("Too wide mask \2%s\2."), parv[1]);
					return;
				}

				// Require non-wildcard top and second level domain
				if (strchr(p, '?') || strchr(p, '*'))
				{
					command_fail(si, fault_badparams, _("Too wide mask \2%s\2."), parv[1]);
					return;
				}

				if (p != host)
				{
					p--;
					while (p >= host && *p != '.')
					{
						if (*p == '?' || *p == '*')
						{
							command_fail(si, fault_badparams, _("Too wide mask \2%s\2."), parv[1]);
							return;
						}
						p--;
					}
				}
			}
		}
		if (myuser_access_find(mu, mask))
		{
			command_fail(si, fault_nochange, _("Mask \2%s\2 is already on your access list."), mask);
			return;
		}
		if (myuser_access_add(mu, mask))
		{
			command_success_nodata(si, _("Added mask \2%s\2 to your access list."), mask);
			logcommand(si, CMDLOG_SET, "ACCESS:ADD: \2%s\2", mask);
		}
		else
			command_fail(si, fault_toomany, _("Your access list is full."));
	}
	else if (!strcasecmp(parv[0], "DEL"))
	{
		if (parc < 2)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS DEL");
			command_fail(si, fault_needmoreparams, _("Syntax: ACCESS DEL <mask>"));
			return;
		}
		mu = si->smu;
		if (mu == NULL)
		{
			command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
			return;
		}
		if ((mask = myuser_access_find(mu, parv[1])) == NULL)
		{
			command_fail(si, fault_nochange, _("Mask \2%s\2 is not on your access list."), parv[1]);
			return;
		}
		command_success_nodata(si, _("Deleted mask \2%s\2 from your access list."), mask);
		logcommand(si, CMDLOG_SET, "ACCESS:DEL: \2%s\2", mask);
		myuser_access_delete_enforce(mu, mask);
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "ACCESS");
		command_fail(si, fault_needmoreparams, _("Syntax: ACCESS ADD|DEL|LIST [mask]"));
		return;
	}
}

static struct command ns_access = {
	.name           = "ACCESS",
	.desc           = N_("Changes and shows your nickname access list."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &ns_cmd_access,
	.help           = { .path = "nickserv/access" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	service_named_bind_command("nickserv", &ns_access);

	use_myuser_access++;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_access);

	use_myuser_access--;
}

SIMPLE_DECLARE_MODULE_V1("nickserv/access", MODULE_UNLOAD_CAPABILITY_OK)
