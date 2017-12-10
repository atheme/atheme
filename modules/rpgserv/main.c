/* main.c - rpgserv main() routine.
 * based on elly's rpgserv for atheme-6.x --nenolod
 */

#include "atheme.h"

service_t *rpgserv;

static void
mod_init(module_t *const restrict m)
{
	rpgserv = service_add("rpgserv", NULL);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	if (rpgserv) {
		service_delete(rpgserv);
		rpgserv = NULL;
	}
}

SIMPLE_DECLARE_MODULE_V1("rpgserv/main", MODULE_UNLOAD_CAPABILITY_OK)
