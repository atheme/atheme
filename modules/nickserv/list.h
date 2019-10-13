/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010-2014 Atheme Project (http://atheme.org/)
 *
 * list.h - /ns list public interface
 *
 * Include this header for modules other than nickserv/list
 * that need to access /ns list functionality.
 */

#ifndef ATHEME_MOD_NICKSERV_LIST_H
#define ATHEME_MOD_NICKSERV_LIST_H 1

#include <atheme.h>
#include "list_common.h"

void (*list_register)(const char *param_name, struct list_param *param);
void (*list_unregister)(const char *param_name);


static inline void use_nslist_main_symbols(struct module *m)
{
    MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/list")
    MODULE_TRY_REQUEST_SYMBOL(m, list_register, "nickserv/list", "list_register")
    MODULE_TRY_REQUEST_SYMBOL(m, list_unregister, "nickserv/list", "list_unregister")
}

#endif /* !ATHEME_MOD_NICKSERV_LIST_H */
