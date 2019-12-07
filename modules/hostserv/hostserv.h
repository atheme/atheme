/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2013 Atheme Project (http://atheme.org/)
 */

#ifndef ATHEME_MOD_HOSTSERV_HOSTSERV_H
#define ATHEME_MOD_HOSTSERV_HOSTSERV_H 1

#include <atheme.h>

/*
 * do_sethost(struct user *u, char *host)
 *
 * Sets a virtual host on a single nickname/user.
 *
 * Inputs:
 *      - User to set a vHost on
 *      - a vHost
 *
 * Outputs:
 *      - none
 *
 * Side Effects:
 *      - The vHost is set on the user.
 */
static inline void do_sethost(struct user *u, const char *host)
{
	struct service *svs;

        if (!strcmp(u->vhost, host ? host : u->host))
                return;

	svs = service_find("hostserv");

        user_sethost(svs->me, u, host ? host : u->host);
}

/*
 * do_sethost_all(struct myuser *mu, char *host)
 *
 * Sets a virtual host on all nicknames in an account.
 *
 * Inputs:
 *      - an account name
 *      - a vHost
 *
 * Outputs:
 *      - none
 *
 * Side Effects:
 *      - The vHost is set on all users logged into
 *        the account.
 */
static inline void do_sethost_all(struct myuser *mu, const char *host)
{
	mowgli_node_t *n;
        struct user *u;

        MOWGLI_ITER_FOREACH(n, mu->logins.head)
        {
                u = n->data;

                do_sethost(u, host);
        }
}

static inline void hs_sethost_all(struct myuser *mu, const char *host, const char *assigner)
{
	mowgli_node_t *n;
	struct mynick *mn;
	char buf[BUFSIZE];
	char timestring[16];

	MOWGLI_ITER_FOREACH(n, mu->nicks.head)
	{
		mn = n->data;
		snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", mn->nick);
		metadata_delete(mu, buf);
	}
	if (host != NULL)
		metadata_add(mu, "private:usercloak", host);
	else
		metadata_delete(mu, "private:usercloak");

	snprintf(timestring, 16, "%lu", (unsigned long)time(NULL));
	metadata_add(mu, "private:usercloak-timestamp", timestring);

	if (assigner != NULL)
		metadata_add(mu, "private:usercloak-assigner", assigner);
	else
		metadata_delete(mu, "private:usercloak-assigner");
}

#endif /* !ATHEME_MOD_HOSTSERV_HOSTSERV_H */
