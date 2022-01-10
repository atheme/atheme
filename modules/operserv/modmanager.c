/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 William Pitcock, et al.
 * Copyright (C) 2021 Atheme Development Group (https://atheme.github.io/)
 *
 * This module replaces modules/operserv/mod{inspect,load,reload,unload} from
 * previous versions of this software.
 *
 * This module cannot be unloadable. If it were, the program would return to
 * functions that no longer exist once the module_unload() call (in either
 * MODRELOAD or MODUNLOAD below) is completed. This would result in a crash.
 *
 * There is very little utility to having MODLOAD be in its own module, too;
 * in previous versions of this software, both MODRELOAD and MODUNLOAD refused
 * to operate on MODLOAD. Finally, MODINSPECT is in this module too, just
 * because it can benefit from the recursive dependency tracking functions
 * added to handle MODRELOAD and MODUNLOAD, when deciding whether a module
 * being inspected is unloadable or not. It would be very confusing to only
 * look at the module's unloadability, tell you it's unloadable, and then have
 * MODRELOAD or MODUNLOAD fail because a permanent module depends upon it.
 */

#include <atheme.h>

#define COLOR_CODE_GREEN     3U
#define COLOR_CODE_RED       4U
#define COLOR_CODE_YELLOW    8U

struct module_dependency
{
	mowgli_node_t                   node;   // For entry into a module dependency list
	const struct module *           mptr;   // Cannot be used after the module is unloaded; for sorting only
	char *                          name;   // A copy of the module name (to be used after it is unloaded)
	enum module_unload_capability   ucap;   // Same purpose as 'name' above
};

static struct service *opersvs = NULL;
static bool os_mi_use_colors = false;

// Wraps the given input in IRC color codes if they are enabled
static const char * ATHEME_FATTR_PRINTF(2, 3)
os_mi_colorize(const unsigned int code, const char *const restrict format, ...)
{
	static char result[BUFSIZE];
	char buf[BUFSIZE];

	va_list argv;
	va_start(argv, format);
	vsnprintf(buf, sizeof buf, format, argv);
	va_end(argv);

	if (os_mi_use_colors)
		(void) snprintf(result, sizeof result, "%s%02u%s%s", "\x03", code, buf, "\x03");
	else
		(void) mowgli_strlcpy(result, buf, sizeof result);

	return result;
}

// Returns a human-readable description of whether a module is unloadable
static inline const char *
os_mi_unloadability_str(const enum module_unload_capability ucap)
{
	switch (ucap)
	{
		case MODULE_UNLOAD_CAPABILITY_OK:
			return os_mi_colorize(COLOR_CODE_GREEN, "%s", _("Unloadable"));
		case MODULE_UNLOAD_CAPABILITY_NEVER:
			return os_mi_colorize(COLOR_CODE_RED, "%s", _("Permanent"));
		case MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY:
			return os_mi_colorize(COLOR_CODE_YELLOW, "%s", _("Reload Only"));
	}

	// To keep gcc happy
	return NULL;
}

// Insert a reverse dependency into a list, skipping duplicates
static inline void
insert_revdep_list_mod(const struct module *const restrict m, mowgli_list_t *const restrict deplist)
{
	const mowgli_node_t *n;
	MOWGLI_LIST_FOREACH(n, deplist->head)
	{
		const struct module_dependency *const mdep = n->data;
		if (strcmp(mdep->name, m->name) == 0)
			return;
	}

	struct module_dependency *const sdep = smalloc(sizeof *sdep);

	(void) mowgli_node_add(sdep, &sdep->node, deplist);

	sdep->mptr = m;
	sdep->name = sstrdup(m->name);
	sdep->ucap = m->can_unload;
}

/* mod_recurse_revdeps()
 *
 * Recurse through a module's reverse dependencies (and their reverse dependencies (and theirs (...)))
 *
 * Inputs:
 *   A loaded module
 *   A list to collect the entire tree of the module's reverse dependencies (optional)
 *   A character pointer to collect the first non-unloadable module name (optional)
 *   A character pointer to collect the first reload-only module name (optional)
 *
 * Returns:
 *   The "worst" unloadability for the tree of module dependencies. If any module is not unloadable at all, that is
 *   returned. Otherwise, if any module is reload-only, that is returned. Otherwise, return that they're unloadable.
 *
 * Side Effects:
 *   If given a list to collect reverse dependencies, insert each module into that list, ignoring duplicates.
 *   If given a character pointer to collect the first non-unloadable module, store the module name there.
 *   If given a character pointer to collect the first reload-only module, store the module name there.
 *
 * WARNING:
 *   The two character pointer arguments MUST be initialised to NULL. They are tested against NULL to see if this is
 *   the first time a value has been stored into them. They MUST NOT be dereferenced after any modules are unloaded.
 */
static enum module_unload_capability
mod_recurse_revdeps(const struct module *const restrict m, mowgli_list_t *const restrict deplist,
                    const char **const restrict n_depname, const char **const restrict r_depname)
{
	enum module_unload_capability hcap = m->can_unload;

	if (n_depname && ! *n_depname && hcap == MODULE_UNLOAD_CAPABILITY_NEVER)
		*n_depname = m->name;

	if (r_depname && ! *r_depname && hcap == MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY)
		*r_depname = m->name;

	if (deplist)
		(void) insert_revdep_list_mod(m, deplist);

	const mowgli_node_t *n;
	MOWGLI_LIST_FOREACH(n, m->required_by.head)
	{
		const enum module_unload_capability rcap = mod_recurse_revdeps(n->data, deplist, n_depname, r_depname);

		if (rcap == MODULE_UNLOAD_CAPABILITY_NEVER)
			hcap = rcap;

		if (rcap == MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY && hcap != MODULE_UNLOAD_CAPABILITY_NEVER)
			hcap = rcap;
	}

	return hcap;
}

/* free_revdep_list()
 *
 * Frees all resources associated with constructing the list of dependencies (see mod_recurse_revdeps())
 */
static inline void
free_revdep_list(mowgli_list_t *const restrict deplist)
{
	const mowgli_node_t *n, *tn;
	MOWGLI_LIST_FOREACH_SAFE(n, tn, deplist->head)
	{
		struct module_dependency *const dep = n->data;

		(void) mowgli_node_delete(&dep->node, deplist);
		(void) sfree(dep->name);
		(void) sfree(dep);
	}

	(void) mowgli_list_free(deplist);
}

/* sort_revdep_list_cb()
 *
 * Callback for mowgli_list_sort() which ensures that if module 1 is required by module 2, module 1 is moved to the
 * left of module 2 in a list of module dependencies.
 *
 * Used in MODRELOAD to ensure that when reloading multiple modules at a time, they are reloaded in the proper order.
 *
 * WARNING:
 *   Assumes no dependency loops, but this is already validated in libathemecore/module.c:module_load(). MUST be
 *   called BEFORE any modules are unloaded; not only will it not account for modules properly if this is not the
 *   case, but it will also access freed memory.
 */
static int
sort_revdep_list_cb(mowgli_node_t *const restrict ln, mowgli_node_t *const restrict rn,
                    void *const restrict ATHEME_VATTR_UNUSED unused)
{
	const struct module *const lm = ((const struct module_dependency *) ln->data)->mptr;
	const struct module *const rm = ((const struct module_dependency *) rn->data)->mptr;

	const mowgli_node_t *n;
	MOWGLI_LIST_FOREACH(n, lm->requires.head)
		if (n->data == rm)
			return 1;

	return 0;
}

/* os_mi_namew_deps()
 *
 * Inputs:
 *   A loaded module
 *   An offset of a dependency list (either forward or reverse) into the loaded module structure
 *   A depth (this is a recursive function -- you must provide zero to start)
 *   A module name length (this is a recursive function -- you must provide zero to start)
 *
 * Returns:
 *   The maximum width of a module name for use in spacing parameters in os_mi_print_deps().
 */
static size_t
os_mi_namew_deps(const struct module *const restrict m, const size_t list_offset, const size_t depth,
                 size_t max_so_far)
{
	// Don't consider the name of the module we're printing dependencies for
	if (depth)
	{
		const size_t name_len = (depth * 2U) + strlen(m->name);
		if (name_len > max_so_far)
			max_so_far = name_len;
	}

	const mowgli_node_t *n;
	const mowgli_list_t *const modlist = (const void *) (((const char *) m) + list_offset);
	MOWGLI_LIST_FOREACH(n, modlist->head)
		max_so_far = os_mi_namew_deps(n->data, list_offset, (depth + 1U), max_so_far);

	return max_so_far;
}

/* os_mi_print_deps()
 *
 * Inputs:
 *   A sourceinfo structure to print modules to
 *   A loaded module
 *   An offset of a dependency list (either forward or reverse) into the loaded module structure
 *   A depth (this is a recursive function -- you must provide zero to start)
 *   A module name length (this is a recursive function -- you must provide zero to start)
 *
 * Side Effects:
 *   Iterate over the module given and all of its dependencies (either forward or reverse), printing each one
 */
static void
os_mi_print_deps(struct sourceinfo *const restrict si, const struct module *const restrict m,
                 const size_t list_offset, const size_t depth, size_t max_name_width)
{
	if (! max_name_width)
		max_name_width = os_mi_namew_deps(m, list_offset, 0U, 0U);

	// printf(3) and co. require an 'int' for a field width specifier parameter
	const int field_width_depth = (int) (depth * 2U);
	const int field_width_names = (int) (max_name_width - (depth * 2U) - 2U);

	const mowgli_node_t *n;
	const mowgli_list_t *const modlist = (const void *) (((const char *) m) + list_offset);
	MOWGLI_LIST_FOREACH(n, modlist->head)
	{
		const struct module *const dm = n->data;
		const enum module_unload_capability ucap = mod_recurse_revdeps(dm, NULL, NULL, NULL);

		(void) command_success_nodata(si, "%-*s- %-*s [%p] (%s)", field_width_depth, "", field_width_names,
		                                  dm->name, dm->address, os_mi_unloadability_str(ucap));

		(void) os_mi_print_deps(si, dm, list_offset, (depth + 1U), max_name_width);
	}
}

// Handle whether a rehash is required due to MODLOAD or MODRELOAD
static void
os_ml_handle_rehash(struct sourceinfo *const restrict si, const bool is_modload)
{
	if (! conf_need_rehash)
		return;

	if (is_modload)
	{
		(void) logcommand(si, CMDLOG_ADMIN, "REHASH (MODLOAD)");
		(void) wallops("Rehashing \2%s\2 to complete module load by request of \2%s\2",
		               config_file, get_oper_name(si));
	}
	else
	{
		(void) logcommand(si, CMDLOG_ADMIN, "REHASH (MODRELOAD)");
		(void) wallops("Rehashing \2%s\2 to complete module reload by request of \2%s\2",
		               config_file, get_oper_name(si));
	}

	if (conf_rehash())
		return;

	(void) command_fail(si, fault_internalerror, _("REHASH of \2%s\2 failed. Please correct any errors "
	                                               "in the file and try again."), config_file);
}

static void
os_cmd_modinspect_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODINSPECT");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: MODINSPECT <module> [module [...]]"));
		return;
	}

	bool listed_any = false;

	for (int i = 0; i < parc; i++)
	{
		const struct module *const m = module_find_published(parv[i]);

		if (! m)
		{
			(void) command_fail(si, fault_nosuch_target, _("\2%s\2 is not loaded."), parv[i]);
			continue;
		}

		(void) logcommand(si, CMDLOG_GET, "MODINSPECT: \2%s\2", m->name);

		(void) command_success_nodata(si, _("Information on \2%s\2:"), m->name);
		(void) command_success_nodata(si, " ");
		(void) command_success_nodata(si, _("Address        : %p"), m->address);

		if (m->header)
		{
			(void) command_success_nodata(si, _("Entry Point    : %p"), m->header->modinit);
			(void) command_success_nodata(si, _("Exit Point     : %p"), m->header->deinit);
			(void) command_success_nodata(si, _("SDK Serial     : %s"), m->header->serial);
			(void) command_success_nodata(si, _("Vendor         : %s"), m->header->vendor);
			(void) command_success_nodata(si, _("Version        : %s"), m->header->version);
		}

		const enum module_unload_capability ucap = mod_recurse_revdeps(m, NULL, NULL, NULL);

		if (m->can_unload == ucap)
			(void) command_success_nodata(si, _("Unloadability  : %s"), os_mi_unloadability_str(ucap));
		else
			(void) command_success_nodata(si, _("Unloadability  : %s (see reverse dependencies below)"),
			                                    os_mi_unloadability_str(ucap));

		if (m->requires.count)
		{
			(void) command_success_nodata(si, " ");
			(void) command_success_nodata(si, _("Dependencies (%zu):"), m->requires.count);
			(void) os_mi_print_deps(si, m, offsetof(struct module, requires), 0U, 0U);
		}
		if (m->required_by.count)
		{
			(void) command_success_nodata(si, " ");
			(void) command_success_nodata(si, _("Reverse Dependencies (%zu):"), m->required_by.count);
			(void) os_mi_print_deps(si, m, offsetof(struct module, required_by), 0U, 0U);
		}

		(void) command_success_nodata(si, " ");

		listed_any = true;
	}

	if (listed_any)
		(void) command_success_nodata(si, _("*** \2End of Info\2 ***"));
}

static void
os_cmd_modload_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODLOAD");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: MODLOAD <module> [module [...]]"));
		return;
	}

	for (int i = 0; i < parc; i++)
	{
		if (module_find_published(parv[i]))
		{
			(void) command_fail(si, fault_nochange, _("\2%s\2 is already loaded."), parv[i]);
			continue;
		}

		(void) logcommand(si, CMDLOG_ADMIN, "MODLOAD: \2%s\2", parv[i]);

		if (module_load(parv[i]))
			(void) command_success_nodata(si, _("Module \2%s\2 loaded."), parv[i]);
		else
			(void) command_fail(si, fault_internalerror, _("Module \2%s\2 failed to load."), parv[i]);
	}

	(void) os_ml_handle_rehash(si, true);
}

static void
os_cmd_modreload_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODRELOAD");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: MODRELOAD <module> [module [...]]"));
		return;
	}

	const char *n_dep = NULL;
	const char *r_dep = NULL;
	bool can_reload_any = false;
	enum module_unload_capability ucap[parc];
	mowgli_list_t *const deplist = mowgli_list_create();

	for (int i = 0; i < parc; i++)
	{
		struct module *const mptr = module_find_published(parv[i]);

		if (! mptr)
		{
			(void) command_fail(si, fault_nochange, _("\2%s\2 is not loaded."), parv[i]);
			continue;
		}
		if ((ucap[i] = mptr->can_unload) == MODULE_UNLOAD_CAPABILITY_NEVER)
		{
			(void) command_fail(si, fault_badparams, _("\2%s\2 is a permanent module; it cannot be "
			                                           "reloaded."), parv[i]);

			(void) slog(LG_ERROR, "MODRELOAD:ERROR: \2%s\2 tried to reload \2%s\2, which is a permanent "
			                      "module", get_oper_name(si), parv[i]);
			continue;
		}
		if ((ucap[i] = mod_recurse_revdeps(mptr, NULL, &n_dep, &r_dep)) == MODULE_UNLOAD_CAPABILITY_NEVER)
		{
			(void) command_fail(si, fault_badparams, _("\2%s\2 is depended upon by \2%s\2, which is a "
			                                           "permanent module and cannot be reloaded."),
			                                           parv[i], n_dep);

			(void) slog(LG_ERROR, "MODRELOAD:ERROR: \2%s\2 tried to reload \2%s\2, which is depended "
			                      "upon by permanent module \2%s\2", get_oper_name(si), parv[i], n_dep);
			continue;
		}

		/* We don't want to put them into the list of modules to reload until now, in case
		 * one of the reverse dependencies of the module given above is not reloadable.
		 */
		(void) mod_recurse_revdeps(mptr, deplist, NULL, NULL);

		(void) logcommand(si, CMDLOG_ADMIN, "MODRELOAD: \2%s\2", parv[i]);

		can_reload_any = true;
	}

	// If we are not able to perform any reloads, just give up now
	if (! can_reload_any)
	{
		(void) free_revdep_list(deplist);
		return;
	}

	/* Sort the list of dependencies so that we reload them in the order they're needed. We can't do this below
	 * because the modules will be unloaded at that point, which is also why we need to iterate parv[] twice...
	 */
	(void) mowgli_list_sort(deplist, &sort_revdep_list_cb, NULL);

	/* If we're reloading a reload-only module, then there's a chance that we'll abort
	 * below (if the module fails to load again). Save the DB beforehand just in case.
	 */
	if (r_dep)
	{
		(void) slog(LG_INFO, "UPDATE (MODRELOAD \2%s\2): \2%s\2", r_dep, get_oper_name(si));
		(void) wallops("Updating database by request of \2%s\2.", get_oper_name(si));
		(void) db_save(NULL, DB_SAVE_BG_IMPORTANT);
	}

	// Now onto the actual unloading and reloading
	for (int i = 0; i < parc; i++)
	{
		// We need to look up the module again in case it was already unloaded as a reverse dependency
		struct module *const mptr = module_find_published(parv[i]);

		if (mptr && ucap[i] != MODULE_UNLOAD_CAPABILITY_NEVER)
			(void) module_unload(mptr, MODULE_UNLOAD_INTENT_RELOAD);
	}

	const mowgli_node_t *n;
	MOWGLI_LIST_FOREACH(n, deplist->head)
	{
		const struct module_dependency *const dep = n->data;

		if (module_find_published(dep->name))
			// Possible it was reloaded already if we're reloading multiple given modules in one command
			continue;

		if (module_load(dep->name))
		{
			(void) command_success_nodata(si, _("Module \2%s\2 reloaded."), dep->name);
			continue;
		}

		if (dep->ucap == MODULE_UNLOAD_CAPABILITY_OK)
		{
			(void) command_fail(si, fault_internalerror, _("Module \2%s\2 failed to reload."), dep->name);

			(void) slog(LG_ERROR, "MODRELOAD:ERROR: \2%s\2 tried to reload \2%s\2; operation failed",
			                      get_oper_name(si), dep->name);
			continue;
		}

		(void) command_fail(si, fault_internalerror, _("Module \2%s\2 failed to reload, and does not allow "
		                                               "unloading. Shutting down to avoid data loss."),
		                                               dep->name);

		(void) slog(LG_ERROR, "MODRELOAD:ERROR: \2%s\2 failed to reload and does not allow unloading. "
		                      "Shutting down to avoid data loss.", dep->name);

		(void) wallops("Shutting down to avoid data loss");
		(void) sendq_flush(curr_uplink->conn);

		exit(EXIT_FAILURE);
	}

	(void) free_revdep_list(deplist);
	(void) os_ml_handle_rehash(si, false);
}

static void
os_cmd_modunload_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODUNLOAD");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: MODUNLOAD <module> [module [...]]"));
		return;
	}

	for (int i = 0; i < parc; i++)
	{
		struct module *const m = module_find_published(parv[i]);

		if (! m)
		{
			(void) command_fail(si, fault_nosuch_target, _("\2%s\2 is not loaded."), parv[i]);
			continue;
		}
		if (m->can_unload != MODULE_UNLOAD_CAPABILITY_OK)
		{
			(void) command_fail(si, fault_badparams, _("\2%s\2 is a permanent module; it cannot be "
			                                           "unloaded."), parv[i]);

			(void) slog(LG_ERROR, "MODUNLOAD:ERROR: \2%s\2 tried to unload \2%s\2, which is a permanent "
			                      "module", get_oper_name(si), parv[i]);
			continue;
		}

		const char *n_dep = NULL;
		const char *r_dep = NULL;
		const enum module_unload_capability ucap = mod_recurse_revdeps(m, NULL, &n_dep, &r_dep);

		if (ucap != MODULE_UNLOAD_CAPABILITY_OK)
		{
			const char *const w_dep = (ucap == MODULE_UNLOAD_CAPABILITY_NEVER) ? n_dep : r_dep;

			(void) command_fail(si, fault_badparams, _("\2%s\2 is depended upon by \2%s\2, which is a "
			                                           "permanent module and cannot be unloaded."),
			                                           parv[i], w_dep);

			(void) slog(LG_ERROR, "MODUNLOAD:ERROR: \2%s\2 tried to unload \2%s\2, which is depended "
			                      "upon by permanent module \2%s\2", get_oper_name(si), parv[i], w_dep);
			continue;
		}

		(void) logcommand(si, CMDLOG_ADMIN, "MODUNLOAD: \2%s\2", parv[i]);
		(void) module_unload(m, MODULE_UNLOAD_INTENT_PERM);
		(void) command_success_nodata(si, _("Module \2%s\2 unloaded."), parv[i]);
	}
}

static struct command os_cmd_modinspect = {
	.name           = "MODINSPECT",
	.desc           = N_("Displays information about loaded modules."),
	.access         = PRIV_SERVER_AUSPEX,
	.maxparc        = 20,
	.cmd            = &os_cmd_modinspect_func,
	.help           = { .path = "oservice/modinspect" },
};

static struct command os_cmd_modload = {
	.name           = "MODLOAD",
	.desc           = N_("Loads modules."),
	.access         = PRIV_ADMIN,
	.maxparc        = 20,
	.cmd            = &os_cmd_modload_func,
	.help           = { .path = "oservice/modload" },
};

static struct command os_cmd_modreload = {
	.name           = "MODRELOAD",
	.desc           = N_("Reloads modules."),
	.access         = PRIV_ADMIN,
	.maxparc        = 20,
	.cmd            = &os_cmd_modreload_func,
	.help           = { .path = "oservice/modreload" },
};

static struct command os_cmd_modunload = {
	.name           = "MODUNLOAD",
	.desc           = N_("Unloads modules."),
	.access         = PRIV_ADMIN,
	.maxparc        = 20,
	.cmd            = &os_cmd_modunload_func,
	.help           = { .path = "oservice/modunload" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	if (! (opersvs = service_find("operserv")))
	{
		(void) slog(LG_ERROR, "%s: service_find() for OperServ failed! (BUG)", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) service_bind_command(opersvs, &os_cmd_modinspect);
	(void) service_bind_command(opersvs, &os_cmd_modload);
	(void) service_bind_command(opersvs, &os_cmd_modreload);
	(void) service_bind_command(opersvs, &os_cmd_modunload);

	(void) add_bool_conf_item("MODINSPECT_USE_COLORS", &opersvs->conf_table, 0, &os_mi_use_colors, false);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Permanent module; nothing to do
}

SIMPLE_DECLARE_MODULE_V1("operserv/modmanager", MODULE_UNLOAD_CAPABILITY_NEVER)
