/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented in doc/LICENSE.
 *
 * This file contains data structures concerning modules.
 */

#ifndef ATHEME_INC_MODULE_H
#define ATHEME_INC_MODULE_H 1

#include "privs.h"
#include "abirev.h"
#include "serno.h"

#define MODTYPE_CORE		1 /* Can't be unloaded. */
#define MODTYPE_FAIL		0x8000 /* modinit failed */

#define MAPI_ATHEME_MAGIC	0xdeadbeef
#define MAPI_ATHEME_V4		4

#define MAX_CMD_PARC		20

enum module_unload_intent
{
	MODULE_UNLOAD_INTENT_PERM            = 0,
	MODULE_UNLOAD_INTENT_RELOAD          = 1,
};

enum module_unload_capability
{
	MODULE_UNLOAD_CAPABILITY_OK          = 0,
	MODULE_UNLOAD_CAPABILITY_NEVER       = 1,
	MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY = 2,
};

/* Module structure. Might be a loaded .so module, or something else that
 * behaves as a module for dependency purposes (perl script, etc).
 */
struct module
{
	char                            name[BUFSIZE];
	char                            modpath[BUFSIZE];
	enum module_unload_capability   can_unload;
	unsigned int                    mflags;
	const struct v4_moduleheader *  header;
	void *                          address;
	mowgli_module_t *               handle;
	void                          (*unload_handler)(struct module *, enum module_unload_intent);
	mowgli_list_t                   dephost;
	mowgli_list_t                   deplist;
	mowgli_list_t                   symlist;
};

struct v4_moduleheader
{
	unsigned int                    atheme_mod;
	unsigned int                    abi_ver;
	unsigned int                    abi_rev;
	const char *                    serial;
	const char *                    name;
	enum module_unload_capability   can_unload;
	void                          (*modinit)(struct module *m);
	void                          (*deinit)(enum module_unload_intent intent);
	const char *                    vendor;
	const char *                    version;
};

/* name is the module name we're searching for.
 * path is the likely full path name, which may be ignored.
 * If it is found, set module to the loaded struct module pointer
 */
typedef struct {
	const char *    name;
	const char *    path;
	struct module * module;
	int             handled;
} hook_module_load_t;

struct module_dependency
{
	char *                          name;
	enum module_unload_capability   can_unload;
};

void modules_init(void);
struct module *module_load(const char *filespec);
void module_load_dir(const char *dirspec);
void module_load_dir_match(const char *dirspec, const char *pattern);
void *module_locate_symbol(const char *modname, const char *sym);
void module_unload(struct module *m, enum module_unload_intent intent);
struct module *module_find(const char *name);
struct module *module_find_published(const char *name);
bool module_request(const char *name);

// Located in libathemecore/module.c
extern mowgli_list_t modules;

#define DECLARE_MODULE_V1(_name, _unloadcap, _modinit, _moddeinit, _ver, _ven)  \
        extern const struct v4_moduleheader _header;                            \
        const struct v4_moduleheader _header = {                                \
                .atheme_mod = MAPI_ATHEME_MAGIC,                                \
                .abi_ver    = MAPI_ATHEME_V4,                                   \
                .abi_rev    = CURRENT_ABI_REVISION,                             \
                .serial     = SERNO,                                            \
                .name       = _name,                                            \
                .can_unload = _unloadcap,                                       \
                .modinit    = _modinit,                                         \
                .deinit     = _moddeinit,                                       \
                .vendor     = _ven,                                             \
                .version    = _ver,                                             \
        }

#define VENDOR_DECLARE_MODULE_V1(name, unloadcap, ven)                     \
        DECLARE_MODULE_V1(name, unloadcap, mod_init, mod_deinit,           \
                          PACKAGE_STRING, ven);

#define SIMPLE_DECLARE_MODULE_V1(name, unloadcap)                          \
        DECLARE_MODULE_V1(name, unloadcap, mod_init, mod_deinit,           \
                          PACKAGE_STRING, VENDOR_STRING);

#define MODULE_TRY_REQUEST_DEPENDENCY(self, modname)                       \
        if (module_request(modname) == false)                              \
        {                                                                  \
                (self)->mflags |= MODTYPE_FAIL;                            \
                return;                                                    \
        }

#define MODULE_TRY_REQUEST_SYMBOL(self, dest, modname, sym)                \
        if ((dest = module_locate_symbol(modname, sym)) == NULL)           \
        {                                                                  \
                MODULE_TRY_REQUEST_DEPENDENCY(self, modname);              \
                if ((dest = module_locate_symbol(modname, sym)) == NULL)   \
                {                                                          \
                        (self)->mflags |= MODTYPE_FAIL;                    \
                        return;                                            \
                }                                                          \
        }

#define MODULE_CONFLICT(self, modname)                                     \
        if (module_find_published(modname))                                \
        {                                                                  \
                slog(LG_ERROR, "module %s conflicts with %s, unloading",   \
                     self->name, modname);                                 \
                (self)->mflags |= MODTYPE_FAIL;                            \
                return;                                                    \
        }

#endif /* !ATHEME_INC_MODULE_H */
