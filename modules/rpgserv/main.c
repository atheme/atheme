/* main.c - rpgserv main() routine.
 * based on elly's rpgserv for atheme-6.x --nenolod
 */

#include "atheme.h"

SIMPLE_DECLARE_MODULE_V1("rpgserv/main", MODULE_UNLOAD_CAPABILITY_OK,
                         _modinit, _moddeinit);

service_t *rpgserv;

void _modinit(module_t *m)
{
	rpgserv = service_add("rpgserv", NULL);
}

void _moddeinit(module_unload_intent_t intent)
{
	if (rpgserv) {
		service_delete(rpgserv);
		rpgserv = NULL;
	}
}
