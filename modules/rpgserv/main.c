/* main.c - rpgserv main() routine.
 * based on elly's rpgserv for atheme-6.x --nenolod
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"rpgserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

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
