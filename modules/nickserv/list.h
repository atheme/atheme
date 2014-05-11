/* list.h - /ns list public interface
 *
 * Include this header for modules other than nickserv/list
 * that need to access /ns list functionality.
 *
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef NSLIST_H
#define NSLIST_H

#include "list_common.h"

void (*list_register)(const char *param_name, list_param_t *param);
void (*list_unregister)(const char *param_name);


static inline void use_nslist_main_symbols(module_t *m)
{
    MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/list");
    MODULE_TRY_REQUEST_SYMBOL(m, list_register, "nickserv/list", "list_register");
    MODULE_TRY_REQUEST_SYMBOL(m, list_unregister, "nickserv/list", "list_unregister");
}

#endif /* !NSLIST_H */
