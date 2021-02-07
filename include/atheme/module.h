/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains data structures concerning modules.
 */

#ifndef ATHEME_INC_MODULE_H
#define ATHEME_INC_MODULE_H 1

#include <atheme/abirev.h>
#include <atheme/constants.h>
#include <atheme/serno.h>
#include <atheme/stdheaders.h>

#define MODFLAG_DBCRYPTO	0x0100U /* starting up without us may leave users unable to login */
#define MODFLAG_DBHANDLER	0x0200U /* starting up without us may mean we can't process the db without data loss */
#define MODFLAG_FAIL		0x8000U /* modinit failed */

#define MAPI_ATHEME_MAGIC	0xDEADBEEFU
#define MAPI_ATHEME_V4		0x00000004U

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
	const void *                    address;
	mowgli_module_t *               handle;
	void                          (*unload_handler)(struct module *, enum module_unload_intent);
	mowgli_list_t                   required_by;
	mowgli_list_t                   requires;
	mowgli_node_t                   mbl_node;
	mowgli_node_t                   mod_node;
};

struct v4_moduleheader
{
	unsigned int                    magic;
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

void modules_init(void);
struct module *module_load(const char *filespec);
void *module_locate_symbol(const char *modname, const char *sym);
void module_unload(struct module *m, enum module_unload_intent intent);
struct module *module_find(const char *name);
struct module *module_find_published(const char *name);
bool module_request(const char *name);

// Located in libathemecore/module.c
extern mowgli_list_t modules;

#define DECLARE_MODULE_V1(_name, _ucap, _modinit, _moddeinit, _ver, _ven)  \
        extern const struct v4_moduleheader _header;                       \
        const struct v4_moduleheader _header = {                           \
                .magic      = MAPI_ATHEME_MAGIC,                           \
                .abi_ver    = MAPI_ATHEME_V4,                              \
                .abi_rev    = CURRENT_ABI_REVISION,                        \
                .serial     = SERNO,                                       \
                .name       = _name,                                       \
                .can_unload = _ucap,                                       \
                .modinit    = _modinit,                                    \
                .deinit     = _moddeinit,                                  \
                .vendor     = _ven,                                        \
                .version    = _ver,                                        \
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
                (self)->mflags |= MODFLAG_FAIL;                            \
                return;                                                    \
        }

#define MODULE_TRY_REQUEST_SYMBOL(self, dest, modname, sym)                \
        MODULE_TRY_REQUEST_DEPENDENCY((self), (modname))                   \
        if (((dest) = module_locate_symbol((modname), (sym))) == NULL)     \
        {                                                                  \
                (self)->mflags |= MODFLAG_FAIL;                            \
                return;                                                    \
        }

#define MODULE_CONFLICT(self, modname)                                     \
        if (module_find_published(modname))                                \
        {                                                                  \
                slog(LG_ERROR, "module %s conflicts with %s, unloading",   \
                     self->name, modname);                                 \
                (self)->mflags |= MODFLAG_FAIL;                            \
                return;                                                    \
        }

#endif /* !ATHEME_INC_MODULE_H */
