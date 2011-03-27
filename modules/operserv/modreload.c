#include "atheme.h"
#include "conf.h"
#include "uplink.h" /* XXX: For sendq_flush and curr_uplink */
#include "datastream.h"

DECLARE_MODULE_V1
(
	"operserv/modreload", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modreload(sourceinfo_t *si, int parc, char *parv[]);

command_t os_modreload = { "MODRELOAD", N_("Reloads a module."), PRIV_ADMIN, 20, os_cmd_modreload, { .path = "oservice/modreload" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_modreload);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &os_modreload);
}

static void recurse_module_deplist(module_t *m, mowgli_list_t *deplist)
{
	mowgli_node_t *n;
	MOWGLI_LIST_FOREACH(n, m->dephost.head)
	{
		module_t *dm = (module_t *) n->data;

		/* Skip duplicates */
		bool found = false;
		mowgli_node_t *n2;
		MOWGLI_LIST_FOREACH(n2, deplist->head)
		{
			module_dependency_t *existing_dep = (module_dependency_t *) n2->data;
			if (0 == strcasecmp(dm->name, existing_dep->name))
				found = true;
		}
		if (found)
			continue;

		module_dependency_t *dep = malloc(sizeof(module_dependency_t));
		dep->name = sstrdup(dm->name);
		dep->can_unload = dm->can_unload;
		mowgli_node_add(dep, mowgli_node_create(), deplist);

		recurse_module_deplist(dm, deplist);
	}
}

static void os_cmd_modreload(sourceinfo_t *si, int parc, char *parv[])
{
	char *module = parv[0];
	module_t *m;
	mowgli_node_t *n;
	module_dependency_t * reloading_semipermanent_module = NULL;

	if (parc < 1)
        {
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODRELOAD");
		command_fail(si, fault_needmoreparams, _("Syntax: MODRELOAD <module...>"));
		return;
        }

	if (!(m = module_find_published(module)))
	{
		command_fail(si, fault_nochange, _("\2%s\2 is not loaded."), module);
		return;
	}

	/* Make sure these checks happen before we construct the list of dependent modules, so that
	 * we can return without having to clean up everything.
	 */

	if (!strcmp(m->name, "operserv/main") || !strcmp(m->name, "operserv/modload") || !strcmp(m->name, "operserv/modunload") || !strcmp(m->name, "operserv/modreload"))
	{
		command_fail(si, fault_noprivs, _("Refusing to reload \2%s\2."), module);
		return;
	}

	if (m->can_unload == MODULE_UNLOAD_CAPABILITY_NEVER)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is a permanent module; it cannot be reloaded."), module);
		slog(LG_ERROR, "MODRELOAD:ERROR: \2%s\2 tried to reload permanent module \2%s\2", get_oper_name(si), module);
		return;
	}

	mowgli_list_t *module_deplist = mowgli_list_create();
	module_dependency_t *self_dep = malloc(sizeof(module_dependency_t));
	self_dep->name = sstrdup(module);
	self_dep->can_unload = m->can_unload;
	mowgli_node_add(self_dep, mowgli_node_create(), module_deplist);
	recurse_module_deplist(m, module_deplist);

	MOWGLI_LIST_FOREACH(n, module_deplist->head)
	{
		module_dependency_t *dep = (module_dependency_t *) n->data;
		if (dep->can_unload == MODULE_UNLOAD_CAPABILITY_NEVER)
		{
			command_fail(si, fault_noprivs, _("\2%s\2 is depended upon by \2%s\2, which is a permanent module and cannot be reloaded."), module, dep->name);
			slog(LG_ERROR, "MODRELOAD:ERROR: \2%s\2 tried to reload \2%s\2, which is depended upon by permanent module \2%s\2", get_oper_name(si), module, dep->name);

			/* We've constructed the dep list now, so don't return without cleaning up */
			while (module_deplist->head != NULL)
			{
				dep = module_deplist->head->data;
				free(dep->name);
				free(dep);
				mowgli_node_delete(module_deplist->head, module_deplist);
				mowgli_list_free(module_deplist);
				return;
			}
		}
		else if (dep->can_unload == MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY
			&& reloading_semipermanent_module == NULL)
		{
			reloading_semipermanent_module = dep;
		}
	}

	if (reloading_semipermanent_module)
	{
		/* If we're reloading a semi-permanent module (reload only; no unload), then there's
		 * a chance that we'll abort if the module fails to load again. Save the DB beforehand
		 * just in case
		 */
		slog(LG_INFO, "UPDATE (due to reload of module \2%s\2): \2%s\2",
				reloading_semipermanent_module->name, get_oper_name(si));
		wallops("Updating database by request of \2%s\2.", get_oper_name(si));
		db_save(NULL);
	}

	module_unload(m, MODULE_UNLOAD_INTENT_RELOAD);

	while (module_deplist->head != NULL)
	{
		module_t *t;
		n = module_deplist->head;
		module_dependency_t *dep = (module_dependency_t *) n->data;

		t = module_load(dep->name);

		if (t != NULL)
		{
			logcommand(si, CMDLOG_ADMIN, "MODRELOAD: \2%s\2 (from \2%s\2)", dep->name, module);
			command_success_nodata(si, _("Module \2%s\2 reloaded (from \2%s\2)."), dep->name, module);
		}
		else
		{
			if (dep->can_unload != MODULE_UNLOAD_CAPABILITY_OK)
			{
				/* Failed to reload a module that can't be unloaded. Abort. */
				command_fail(si, fault_nosuch_target, _(
						"Module \2%s\2 failed to reload, and does not allow unloading. "
						"Shutting down to avoid data loss."), dep->name);
				slog(LG_ERROR, "MODRELOAD:ERROR: \2%s\2 failed to reload and does not allow unloading. "
						"Shutting down to avoid data loss.", dep->name);
				sendq_flush(curr_uplink->conn);

				exit(EXIT_FAILURE);
			}

			command_fail(si, fault_nosuch_target, _("Module \2%s\2 failed to reload."), dep->name);
			slog(LG_ERROR, "MODRELOAD:ERROR: \2%s\2 tried to reload \2%s\2 (from \2%s\2), operation failed.", get_oper_name(si), dep->name, module);
		}

		free(dep->name);
		free(dep);
		mowgli_node_delete(n, module_deplist);
	}
	mowgli_list_free(module_deplist);

	if (conf_need_rehash)
	{
		logcommand(si, CMDLOG_ADMIN, "REHASH (MODRELOAD)");
		wallops("Rehashing \2%s\2 to complete module reload by request of \2%s\2.", config_file, get_oper_name(si));
		if (!conf_rehash())
			command_fail(si, fault_nosuch_target, _("REHASH of \2%s\2 failed. Please correct any errors in the file and try again."), config_file);
	}
}
