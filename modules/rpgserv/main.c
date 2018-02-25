/* main.c - rpgserv main() routine.
 * based on elly's rpgserv for atheme-6.x --nenolod
 */

#include "atheme.h"

static struct service *rpgserv = NULL;

static void
mod_init(struct module *const restrict m)
{
	rpgserv = service_add("rpgserv", NULL);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	if (rpgserv) {
		service_delete(rpgserv);
		rpgserv = NULL;
	}
}

SIMPLE_DECLARE_MODULE_V1("rpgserv/main", MODULE_UNLOAD_CAPABILITY_OK)
