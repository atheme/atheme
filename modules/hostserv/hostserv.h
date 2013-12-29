#ifndef INLINE_HOSTSERV_H
#define INLINE_HOSTSERV_H

typedef struct {
        const char *host;
	sourceinfo_t *si;
	int approved;
	const char *target;
} hook_host_request_t;

/*
 * do_sethost(user_t *u, char *host)
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
static inline void do_sethost(user_t *u, const char *host)
{
	service_t *svs;

        if (!strcmp(u->vhost, host ? host : u->host))
                return;

	svs = service_find("hostserv");

        user_sethost(svs->me, u, host ? host : u->host);
}

/*
 * do_sethost_all(myuser_t *mu, char *host)
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
static inline void do_sethost_all(myuser_t *mu, const char *host)
{
	mowgli_node_t *n;
        user_t *u;

        MOWGLI_ITER_FOREACH(n, mu->logins.head)
        {
                u = n->data;

                do_sethost(u, host);
        }
}

static inline void hs_sethost_all(myuser_t *mu, const char *host, const char *assigner)
{
	mowgli_node_t *n;
	mynick_t *mn;
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

	snprintf(timestring, 16, "%d", time(NULL));
	metadata_add(mu, "private:usercloak-timestamp", timestring);

	if (assigner != NULL)
		metadata_add(mu, "private:usercloak-assigner", assigner);
	else
		metadata_delete(mu, "private:usercloak-assigner");
}
#endif
