/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Sets usercloak metadata on register.
 *
 * $Id: gen_vhostonreg.c 6331 2006-09-08 23:14:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"misc/vhostonreg", FALSE, _modinit, _moddeinit,
	"$Revision: 6331 $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void handle_register(void *vptr);

void _modinit(module_t *m)
{
	hook_add_event("user_register");
	hook_add_hook("user_register", handle_register);
}

void _moddeinit(void)
{
	hook_del_hook("user_register", handle_register);
}

static void handle_register(void *vptr)
{
	myuser_t *mu = vptr;
	node_t *n;
	user_t *u;
	char buf[BUFSIZE];

	if (me.hidehostsuffix == NULL)
		return;

	snprintf(buf, BUFSIZE, "%s.%s", mu->name, me.hidehostsuffix);

	metadata_add(mu, METADATA_USER, "private:usercloak", buf);

	LIST_FOREACH(n, mu->logins.head)
	{
		u = n->data;
		hook_call_event("user_identify", u); /* XXX */
	}
}
