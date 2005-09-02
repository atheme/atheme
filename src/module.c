/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Module management.
 *
 * $Id: module.c 2057 2005-09-02 06:40:10Z nenolod $
 */

#include "atheme.h"
#include <dlfcn.h>

static BlockHeap *module_heap;
list_t modules;

/* Microsoft's POSIX API is a joke. */
#ifdef _WIN32

#define dirent dirent
#define DIR lt_DIR

struct dirent
{
	char d_name[2048];
	int  d_namlen;
};

typedef struct _DIR
{
	HANDLE hSearch;
	WIN32_FIND_DATA Win32FindData;
	BOOL firsttime;
	struct dirent file_info;
} DIR;

static void closedir(DIR *entry)
{
	FindClose(entry->hSearch);
	free(entry);
}

static DIR* opendir(const char *path)
{
	char filespec[2048];
	DIR *entry;

	strncpy (filespec, path, 2048);
	filespec[strlen(filespec)] = '\0';
	strcat(filespec, "\\");
	entry = malloc(sizeof(DIR));

	if (entry != NULL)
	{
		entry->firsttime = TRUE;
		entry->hSearch = FindFirstFile(filespec, &entry->Win32FindData);
	}

	if (entry->hSearch == INVALID_HANDLE_VALUE)
	{
		strcat(filespec, "\\*.*");
		entry->hSearch = FindFirstFile(filespec, &entry->Win32FindData);

		if (entry->hSearch == INVALID_HANDLE_VALUE)
		{
			free (entry);
			return NULL;
		}
	}

	return entry;
}

static struct dirent *readdir(DIR *entry)
{
	int status;

	if (!entry)
		return NULL;

	if (!entry->firsttime)
	{
		status = FindNextFile(entry->hSearch, &entry->Win32FindData);

		if (status == 0)
			return NULL;
	}

	entry->firsttime = FALSE;
	strncpy(entry->file_info.d_name,entry->Win32FindData.cFileName,
		2048);

	entry->file_info.d_name[2048] = '\0';
	entry->file_info.d_namlen = strlen(entry->file_info.d_name);

	return &entry->file_info;
}

#endif

void modules_init(void)
{
	module_heap = BlockHeapCreate(sizeof(module_t), 256);

	if (!module_heap)
	{
		slog(LG_INFO, "modules_init(): block allocator failed.");
		exit(EXIT_FAILURE);
	}

	module_load_dir(PREFIX "/modules");
}

/*
 * module_load()
 *
 * inputs:
 *       a literal filename for a module to load.
 *
 * outputs:
 *       the respective module_t object of the module.
 *
 * side effects:
 *       a module is loaded and necessary initialization code is ran.
 */
module_t *module_load(char *filespec)
{
	node_t *n;
	module_t *m;
	moduleheader_t *h;
	void *handle = NULL;

	if ((m = module_find(filespec)))
	{
		slog(LG_INFO, "module_load(): module %s is already loaded at [0x%lx]",
				modname, m->address);
		return NULL;
	}

	handle = linker_open_ext(filespec);

	if (!handle)
	{
		char *errp = sstrdup(dlerror());
		slog(LG_INFO, "module_load(): error: %s", errp);
		wallops("Error while loading module %s: %s", modname, errp);
		free(errp);
		return NULL;
	}

	h = (moduleheader_t *) linker_getsym(handle, "_header");

	if (!h)
		return NULL;

	if (h->atheme_mod != MAPI_ATHEME_MAGIC)
	{
		slog(LG_DEBUG, "module_load(): %s: Attempted to load an incompatible module. Aborting.", modname);

		if (me.connected)
			wallops("Module %s is not a valid atheme module.", modname);

		linker_close(handle);
		return NULL;
	}

	slog(LG_DEBUG, "module_load(): loaded %s at [0x%lx; MAPI version %d]", modname, handle, h->abi_ver);

	if (me.connected)
		wallops("Module %s loaded at [0x%lx; MAPI version %d]", modname, handle, h->abi_ver);

	m = BlockHeapAlloc(module_heap);

	strlcpy(m->modpath, filespec, BUFSIZE);
	m->address = handle;
	m->mflags = MODTYPE_STANDARD;
	m->header = h;

	n = node_create();

	node_add(m, n, &modules);

	if (h->modinit)
		h->modinit(m);

	return m;
}

/*
 * module_load_dir()
 *
 * inputs:
 *       a directory containing modules to load.
 *
 * outputs:
 *       none
 *
 * side effects:
 *       qualifying modules are passed to module_load().
 */
void module_load_dir(char *dirspec)
{
	DIR *module_dir = NULL;
	struct dirent *ldirent = NULL;
	char module_filename[4096];

	module_dir = opendir(dirspec);

	if (!module_dir)
	{
		slog(LG_INFO, "module_load_dir(): %s: %s", dirspec, strerror(errno));
		return;
	}

	while ((ldirent = readdir(module_dir)) != NULL)
	{
		if (!strstr(ldirent->d_name, ".so"))
			continue;

		snprintf(module_filename, sizeof(module_filename), "%s/%s", dirspec, ldirent->d_name);
		module_load(module_filename);
	}

	closedir(module_dir);
}

/*
 * module_load_dir_match()
 *
 * inputs:
 *       a directory containing modules to load, and a pattern to match against
 *       to determine whether or not a module qualifies for loading.
 *
 * outputs:
 *       none
 *
 * side effects:
 *       qualifying modules are passed to module_load().
 */
void module_load_dir_match(char *dirspec, char *pattern)
{
	DIR *module_dir = NULL;
	struct dirent *ldirent = NULL;
	char module_filename[4096];

	module_dir = opendir(dirspec);

	if (!module_dir)
	{
		slog(LG_INFO, "module_load_dir(): %s: %s", dirspec, strerror(errno));
		return;
	}

	while ((ldirent = readdir(module_dir)) != NULL)
	{
		if (!strstr(ldirent->d_name, ".so") && match(pattern, ldirent->d_name))
			continue;

		snprintf(module_filename, sizeof(module_filename), "%s/%s", dirspec, ldirent->d_name);
		module_load(module_filename);
	}

	closedir(module_dir);
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
 *       a module is unloaded and neccessary deinitalization code is ran.
 */
void module_unload(module_t *m)
{
	node_t *n;

	if (!m)
		return;

	slog(LG_INFO, "module_unload(): unloaded %s", m->name);

	n = node_find(m, &modules);

	if (m->header->deinit)
		m->header->deinit();

	linker_close(m->address);

	node_del(n, &modules);

	BlockHeapFree(module_heap, m);
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
void *module_locate_symbol(char *modname, char *sym)
{
	module_t *m;
	void *symptr;

	if (!(m = module_find_published(modname)))
	{
		slog(LG_DEBUG, "module_locate_symbol(): %s is not loaded.", modname);
		return NULL;
	}

	symptr = linker_getsym(m->address, sym);

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
module_t *module_find(char *name)
{
	node_t *n;

	LIST_FOREACH(n, modules.head)
	{
		module_t *m = n->data;

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
module_t *module_find_published(char *name)
{
        node_t *n;

        LIST_FOREACH(n, modules.head)
        {
                module_t *m = n->data;

                if (!strcasecmp(m->header->name, name))
                        return m;
        }

        return NULL;
}

