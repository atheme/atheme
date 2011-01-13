/* chanfix - channel fixing service
 * Copyright (c) 2010 Atheme Development Group
 */

#include "atheme.h"
#include "chanfix.h"

DECLARE_MODULE_V1
(
	"chanfix/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

service_t *chanfix;
mowgli_list_t conf_cfx_table;

void _modinit(module_t *m)
{
	chanfix_persist_record_t *rec = mowgli_global_storage_get("atheme.chanfix.main.persist");

	chanfix_gather_init(rec);

	if (rec != NULL)
	{
		free(rec);
		return;
	}

	chanfix = service_add("chanfix", NULL, &conf_cfx_table);
	service_bind_command(chanfix, &cmd_chanfix);
	service_bind_command(chanfix, &cmd_scores);
	service_bind_command(chanfix, &cmd_info);
	service_bind_command(chanfix, &cmd_help);

	event_add("chanfix_autofix", chanfix_autofix_ev, NULL, 60);
}

void _moddeinit(module_unload_intent_t intent)
{
	chanfix_persist_record_t *rec = NULL;

	event_delete(chanfix_autofix_ev, NULL);

	if (chanfix)
		service_delete(chanfix);

	switch (intent)
	{
		case MODULE_UNLOAD_INTENT_RELOAD:
		{
			rec = smalloc(sizeof(chanfix_persist_record_t));
			rec->version = 1;

			mowgli_global_storage_put("atheme.chanfix.main.persist", rec);
			break;
		}

		case MODULE_UNLOAD_INTENT_PERM:
		default:
			break;
	}

	chanfix_gather_deinit(intent, rec);
}
