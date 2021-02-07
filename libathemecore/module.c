/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * module.c: Module management.
 */

#include <atheme.h>
#include "internal.h"

#ifdef HAVE_USABLE_DLINFO
#  include <dlfcn.h>
#  include <link.h>
#endif

static mowgli_list_t modules_being_loaded;
static mowgli_heap_t *module_heap = NULL;
static struct module *current_module = NULL;

mowgli_list_t modules;

void
modules_init(void)
{
	if (! (module_heap = sharedheap_get(sizeof(struct module))))
	{
		slog(LG_ERROR, "modules_init(): block allocator failed.");
		exit(EXIT_FAILURE);
	}
}

static inline void
module_add_dependency(struct module *const restrict m)
{
	if (m && current_module && ! mowgli_node_find(m, &current_module->requires))
	{
		(void) mowgli_node_add(m, mowgli_node_create(), &current_module->requires);
		(void) mowgli_node_add(current_module, mowgli_node_create(), &m->required_by);

		(void) slog(LG_DEBUG, "%s: \2%s\2 added as a dependency of \2%s\2",
		                      MOWGLI_FUNC_NAME, m->name, current_module->name);
	}
}

/*
 * module_load_internal: the part of module_load that deals with 'real' shared
 * object modules.
 */
static struct module *
module_load_internal(const char *const restrict pathname, char *const restrict errbuf, const size_t errlen)
{
	char linker_errbuf[BUFSIZE];
	mowgli_module_t *const handle = linker_open_ext(pathname, linker_errbuf, BUFSIZE);

	if (!handle)
	{
		snprintf(errbuf, errlen, "module_load(): error while loading %s: \2%s\2", pathname, linker_errbuf);
		return NULL;
	}

	const struct v4_moduleheader *const h = mowgli_module_symbol(handle, "_header");

	if (h == NULL || h->magic != MAPI_ATHEME_MAGIC)
	{
		snprintf(errbuf, errlen, "module_load(): \2%s\2: Attempted to load an incompatible module. Aborting.", pathname);

		mowgli_module_close(handle);
		return NULL;
	}
	if (h->abi_ver != MAPI_ATHEME_V4)
	{
		snprintf(errbuf, errlen, "module_load(): \2%s\2: MAPI version mismatch (%u != %u), please recompile.", pathname, h->abi_ver, MAPI_ATHEME_V4);

		mowgli_module_close(handle);
		return NULL;
	}
	if (h->abi_rev != CURRENT_ABI_REVISION)
	{
		snprintf(errbuf, errlen, "module_load(): \2%s\2: ABI revision mismatch (%u != %u), please recompile.", pathname, h->abi_rev, CURRENT_ABI_REVISION);

		mowgli_module_close(handle);
		return NULL;
	}
	if (module_find_published(h->name))
	{
		snprintf(errbuf, errlen, "module_load(): \2%s\2: Published name \2%s\2 already exists.", pathname, h->name);

		mowgli_module_close(handle);
		return NULL;
	}

	struct module *const m = mowgli_heap_alloc(module_heap);

	mowgli_strlcpy(m->modpath, pathname, BUFSIZE);
	mowgli_strlcpy(m->name, h->name, BUFSIZE);

	m->can_unload   = h->can_unload;
	m->handle       = handle;
	m->mflags       = 0;
	m->header       = h;

#ifdef HAVE_USABLE_DLINFO
	struct link_map *map = NULL;
	if (dlinfo(handle, RTLD_DI_LINKMAP, &map) == 0 && map && map->l_addr)
		m->address = (const void *) map->l_addr;
	else
		m->address = handle;
#else
	/* best we can do here without dlinfo() --nenolod */
	m->address = handle;
#endif

	if (h->modinit)
	{
		/* The only permitted mechanism of importing a module as a
		 * dependency is within the modinit function of a real shared
		 * module, so all of this logic can be contained within this
		 * block:
		 *
		 *   1) Save a copy of the pointer to the module that is
		 *      currently being initialised (if any).
		 *
		 *   2) Set that pointer to this module. This is necessary to
		 *      know which module is depending on another in step 4.
		 *
		 *   3) Add this module to the list of modules being loaded.
		 *      This is necessary to detect circular dependencies in
		 *      step 4.
		 *
		 *   4) Call this module's modinit function. This may end up
		 *      recursing into this logic again, if it requests any
		 *      other modules as dependencies.
		 *
		 *   5) Remove this module from the list of modules being
		 *      loaded.
		 *
		 *   6) Restore the old value of the pointer to the module
		 *      currently being initialised. This is necessary to
		 *      allow for the previous module to import yet more
		 *      modules as extra dependencies, and to finally clear
		 *      the variable when there is no previous module to go
		 *      back to (end of recursion in step 4).
		 *
		 *   -- amdj
		 */
		struct module *const cmt = current_module;

		current_module = m;

		(void) mowgli_node_add(m, &m->mbl_node, &modules_being_loaded);
		(void) h->modinit(m);
		(void) mowgli_node_delete(&m->mbl_node, &modules_being_loaded);

		current_module = cmt;
	}

	if (m->mflags & MODFLAG_FAIL)
	{
		snprintf(errbuf, errlen, "module_load(): module \2%s\2 init failed", pathname);
		module_unload(m, MODULE_UNLOAD_INTENT_PERM);
		return NULL;
	}

	slog(LG_DEBUG, "module_load(): loaded %s [at %p; MAPI version %u]", h->name, m->address, h->abi_ver);

	return m;
}

/*
 * module_load()
 *
 * inputs:
 *       a literal filename for a module to load.
 *
 * outputs:
 *       the respective struct module object of the module.
 *
 * side effects:
 *       a module, or module-like object, is loaded and necessary initialization
 *       code is run.
 */
struct module *
module_load(const char *const restrict filespec)
{
	char pathbuf[BUFSIZE];
	const char *pathname = filespec;

	/* / or C:\... */
	if (! (*filespec == '/' || *(filespec + 1) == ':'))
	{
		snprintf(pathbuf, BUFSIZE, "%s/%s", MODDIR "/modules", filespec);
		slog(LG_DEBUG, "module_load(): translated %s to %s", filespec, pathbuf);
		pathname = pathbuf;
	}

	struct module *m;
	if ((m = module_find(pathname)))
	{
		slog(LG_VERBOSE, "module_load(): module \2%s\2 is already loaded [at %p]", pathname, m->address);
		return NULL;
	}

	char errbuf[BUFSIZE];
	if (! (m = module_load_internal(pathname, errbuf, sizeof errbuf)))
	{
		struct hook_module_load hdata = {
			.name       = filespec,
			.path       = pathname,
			.module     = NULL,
			.handled    = 0,
		};

		hook_call_module_load(&hdata);

		if (! hdata.module)
		{
			if (!hdata.handled)
				slog(LG_ERROR, "%s", errbuf);

			return NULL;
		}

		m = hdata.module;
	}

	mowgli_node_add(m, mowgli_node_create(), &modules);

	if (me.connected && !cold_start)
	{
		wallops("Module %s loaded at %p", m->name, m->address);
		slog(LG_INFO, "MODLOAD: \2%s\2 at %p", m->name, m->address);
	}

	return m;
}

/*
 * module_unload()
 *
 * inputs:
 *       a module object to unload.
 *
 * outputs:
 *       none
 *
 * side effects:
 *       a module is unloaded and neccessary deinitalization code is run.
 */
void
module_unload(struct module *const restrict m, const enum module_unload_intent intent)
{
	mowgli_node_t *n, *tn;

	if (!m)
		return;

	/* unload modules which depend on us */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, m->required_by.head)
	{
		struct module *const hm = (struct module *) n->data;

		module_unload(hm, intent);
	}

	/* let modules that we depend on know that we no longer exist */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, m->requires.head)
	{
		struct module *const hm = (struct module *) n->data;
		mowgli_node_t *const hn = mowgli_node_find(m, &hm->required_by);

		continue_if_fail(hn != NULL);

		mowgli_node_delete(hn, &hm->required_by);
		mowgli_node_free(hn);
		mowgli_node_delete(n, &m->requires);
		mowgli_node_free(n);
	}

	n = mowgli_node_find(m, &modules);
	if (n != NULL)
	{
		if (m->header && m->header->deinit)
			m->header->deinit(intent);

		mowgli_node_delete(n, &modules);
		mowgli_node_free(n);

		(void) slog(LG_INFO, "%s: unloaded \2%s\2", MOWGLI_FUNC_NAME, m->name);

		if (me.connected)
			(void) wallops("Module %s unloaded.", m->name);
	}

	/* else unloaded in embryonic state */
	if (m->handle)
	{
		mowgli_module_close(m->handle);
		mowgli_heap_free(module_heap, m);
	}
	else
	{
		/* If handle is null, unload_handler is required to be valid
		 * and we can't do anything meaningful if it isn't.
		 */
		m->unload_handler(m, intent);
	}
}

/*
 * module_locate_symbol()
 *
 * inputs:
 *       a name of a module and a symbol to look for inside it.
 *
 * outputs:
 *       the pointer to the module symbol, else NULL if not found.
 *
 * side effects:
 *       none
 */
void *
module_locate_symbol(const char *const restrict modname, const char *const restrict sym)
{
	struct module *m;
	if (!(m = module_find_published(modname)))
	{
		slog(LG_ERROR, "module_locate_symbol(): %s is not loaded.", modname);
		return NULL;
	}

	/* If this isn't a loaded .so module, we can't search for symbols in it
	 */
	if (!m->handle)
		return NULL;

	void *const symptr = mowgli_module_symbol(m->handle, sym);
	if (symptr == NULL)
	{
		slog(LG_ERROR, "module_locate_symbol(): could not find symbol %s in module %s.", sym, modname);
		return NULL;
	}

	/* This shouldn't be necessary, because the recommended way to import symbols is with
	 * MODULE_TRY_REQUEST_SYMBOL, which first does MODULE_TRY_REQUEST_DEPENDENCY, which does
	 * module_request(), which will add it as a dependency. Still, best to be thorough.
	 *
	 *   -- amdj
	 */
	(void) module_add_dependency(m);

	return symptr;
}

/*
 * module_find()
 *
 * inputs:
 *       a name of a module to locate the object for.
 *
 * outputs:
 *       the module object if the module is located, else NULL.
 *
 * side effects:
 *       none
 */
struct module *
module_find(const char *const restrict name)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, modules.head)
	{
		struct module *const m = n->data;

		if (!strcasecmp(m->modpath, name))
			return m;
	}

	return NULL;
}

/*
 * module_find_published()
 *
 * inputs:
 *       a published (in _header) name of a module to locate the object for.
 *
 * outputs:
 *       the module object if the module is located, else NULL.
 *
 * side effects:
 *       none
 */
struct module *
module_find_published(const char *const restrict name)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, modules.head)
	{
		struct module *const m = n->data;

		if (!strcasecmp(m->name, name))
			return m;
	}

	return NULL;
}

/*
 * module_request()
 *
 * inputs:
 *       a published name of a module to load.
 *
 * outputs:
 *       true: if the module was loaded, or is already present.
 *
 * side effects:
 *       a module might be loaded.
 */
bool
module_request(const char *const restrict name)
{
	struct module *m;
	mowgli_node_t *n;

	if ((m = module_find_published(name)))
	{
		(void) module_add_dependency(m);
		return true;
	}
	MOWGLI_ITER_FOREACH(n, modules_being_loaded.head)
	{
		m = n->data;

		if (!strcasecmp(m->name, name))
		{
			slog(LG_ERROR, "module_request(): circular dependency between modules %s and %s",
					current_module->name, m->name);
			return false;
		}
	}
	if ((m = module_load(name)))
	{
		(void) module_add_dependency(m);
		return true;
	}

	return false;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
