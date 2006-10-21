/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Changes and shows nickname access lists.
 *
 * $Id: access.c 6791 2006-10-21 16:59:20Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/access", FALSE, _modinit, _moddeinit,
	"$Id: access.c 6791 2006-10-21 16:59:20Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_access(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_access = { "ACCESS", "Changes and shows your nickname access list.", AC_NONE, 2, ns_cmd_access };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_access, ns_cmdtree);
	help_addentry(ns_helptree, "ACCESS", "help/nickserv/access", NULL);

	use_myuser_access++;
}

void _moddeinit()
{
	command_delete(&ns_access, ns_cmdtree);
	help_delentry(ns_helptree, "ACCESS");

	use_myuser_access--;
}

static void ns_cmd_access(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	node_t *n;
	char *mask;
	char *host;
	char *p;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS");
		command_fail(si, fault_needmoreparams, "Syntax: ACCESS ADD|DEL|LIST [mask]");
		return;
	}

	if (!strcasecmp(parv[0], "LIST"))
	{
		if (parc < 2)
		{
			mu = si->smu;
			if (mu == NULL)
			{
				command_fail(si, fault_noprivs, "You are not logged in.");
				return;
			}
		}
		else
		{
			if (!has_priv(si, PRIV_USER_AUSPEX))
			{
				command_fail(si, fault_noprivs, "You are not authorized to use the target argument.");
				return;
			}

			if (!(mu = myuser_find_ext(parv[1])))
			{
				command_fail(si, fault_badparams, "\2%s\2 is not registered.", parv[1]);
				return;
			}
		}

		if (mu != si->smu)
			logcommand(si, CMDLOG_ADMIN, "ACCESS LIST %s", mu->name);
		else
			logcommand(si, CMDLOG_GET, "ACCESS LIST");

		command_success_nodata(si, "Access list for \2%s\2:", mu->name);

		LIST_FOREACH(n, mu->access_list.head)
		{
			mask = n->data;
			command_success_nodata(si, "- %s", mask);
		}

		command_success_nodata(si, "End of \2%s\2 access list.", mu->name);
	}
	else if (!strcasecmp(parv[0], "ADD"))
	{
		if (parc < 2)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS ADD");
			command_fail(si, fault_needmoreparams, "Syntax: ACCESS ADD <mask>");
			return;
		}
		mu = si->smu;
		mask = parv[1];
		if (mu == NULL)
		{
			command_fail(si, fault_noprivs, "You are not logged in.");
			return;
		}
		if (mask[0] == '*' && mask[1] == '!')
			mask += 2;
		if (strlen(mask) >= USERLEN + HOSTLEN)
		{
			command_fail(si, fault_badparams, "Invalid mask \2%s\2.", parv[1]);
			return;
		}
		p = mask;
		while (*p != '\0')
		{
			if (!isprint(*p) || *p == ' ' || *p == '!')
			{
				command_fail(si, fault_badparams, "Invalid mask \2%s\2.", parv[1]);
				return;
			}
			p++;
		}
		host = strchr(mask, '@');
		if (host == NULL) /* account name access masks? */
		{
			command_fail(si, fault_badparams, "Invalid mask \2%s\2.", parv[1]);
			return;
		}
		host++;
		if (si->su != NULL && (!strcasecmp(host, si->su->host) || !strcasecmp(host, si->su->vhost)))
			; /* it's their host, allow it */
		else if (host[0] == '.' || host[0] == ':' || host[0] == '\0' || host[1] == '\0' || host == mask + 1 || strchr(host, '@') || strstr(host, ".."))
		{
			command_fail(si, fault_badparams, "Invalid mask \2%s\2.", parv[1]);
			return;
		}
		else if ((strchr(host, '*') || strchr(host, '?')) && (mask[0] == '*' && mask[1] == '@'))
		{
			/* can't use * username and wildcarded host */
			command_fail(si, fault_badparams, "Too wide mask \2%s\2.", parv[1]);
			return;
		}
		else if ((p = strrchr(host, '/')) != NULL)
		{
			if (isdigit(p[1]) && (atoi(p + 1) < 16 || (mask[0] == '*' && mask[1] == '@')))
			{
				command_fail(si, fault_badparams, "Too wide mask \2%s\2.", parv[1]);
				return;
			}
			if (p[0] == '*')
			{
				command_fail(si, fault_badparams, "Too wide mask \2%s\2.", parv[1]);
				return;
			}
		}
		else
		{
			p = strrchr(host, '.');
			if (p != NULL)
			{
				/* Require non-wildcard top and second level
				 * domain */
				if (strchr(p, '?') || strchr(p, '*'))
				{
					command_fail(si, fault_badparams, "Too wide mask \2%s\2.", parv[1]);
					return;
				}
				p--;
				while (p >= host && *p != '.')
				{
					if (*p == '?' || *p == '*')
					{
						command_fail(si, fault_badparams, "Too wide mask \2%s\2.", parv[1]);
						return;
					}
					p--;
				}
			}
			else if (strchr(host, ':'))
			{
				/* No wildcarded IPs */
				if (strchr(host, '?') || strchr(host, '*'))
				{
					command_fail(si, fault_badparams, "Too wide mask \2%s\2.", parv[1]);
					return;
				}
			}
			else /* no '.' or ':' */
			{
				command_fail(si, fault_badparams, "Invalid mask \2%s\2.", parv[1]);
				return;
			}
		}
		if (myuser_access_find(mu, mask))
		{
			command_fail(si, fault_nochange, "Mask \2%s\2 is already on your access list.", mask);
			return;
		}
		if (myuser_access_add(mu, mask))
		{
			command_success_nodata(si, "Added mask \2%s\2 to your access list.", mask);
			logcommand(si, CMDLOG_SET, "ACCESS ADD %s", mask);
		}
		else
			command_fail(si, fault_toomany, "Your access list is full.");
	}
	else if (!strcasecmp(parv[0], "DEL"))
	{
		if (parc < 2)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS DEL");
			command_fail(si, fault_needmoreparams, "Syntax: ACCESS DEL <mask>");
			return;
		}
		mu = si->smu;
		if (mu == NULL)
		{
			command_fail(si, fault_noprivs, "You are not logged in.");
			return;
		}
		if ((mask = myuser_access_find(mu, parv[1])) == NULL)
		{
			command_fail(si, fault_nochange, "Mask \2%s\2 is not on your access list.", parv[1]);
			return;
		}
		command_success_nodata(si, "Deleted mask \2%s\2 from your access list.", mask);
		logcommand(si, CMDLOG_SET, "ACCESS DEL %s", mask);
		myuser_access_delete(mu, mask);
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "ACCESS");
		command_fail(si, fault_needmoreparams, "Syntax: ACCESS ADD|DEL|LIST [mask]");
		return;
	}
}
