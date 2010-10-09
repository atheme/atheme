/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 */

#include "atheme.h"
#include "hostserv.h"

DECLARE_MODULE_V1
(
	"hostserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void on_user_identify(user_t *u);

service_t *hostsvs;
mowgli_list_t conf_hs_table;

void _modinit(module_t *m)
{
	hook_add_event("user_identify");
	hook_add_user_identify(on_user_identify);

	hostsvs = service_add("hostserv", NULL, &conf_hs_table);
}

void _moddeinit(void)
{
	if (hostsvs != NULL)
		service_delete(hostsvs);

	hook_del_user_identify(on_user_identify);
}

static void on_user_identify(user_t *u)
{
	myuser_t *mu = u->myuser;
	metadata_t *md;
	char buf[NICKLEN + 20];

	snprintf(buf, sizeof buf, "private:usercloak:%s", u->nick);
	md = metadata_find(mu, buf);
	if (md == NULL)
		md = metadata_find(mu, "private:usercloak");
	if (md == NULL)
		return;

	do_sethost(u, md->value);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
