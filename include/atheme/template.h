/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Jilles Tjoelker, et al.
 *
 * Predefined flags collections
 */

#ifndef ATHEME_INC_TEMPLATE_H
#define ATHEME_INC_TEMPLATE_H 1

#include <atheme/stdheaders.h>
#include <atheme/structures.h>

struct default_template
{
	unsigned int    flags;
};

const char *getitem(const char *str, const char *name);
unsigned int get_template_flags(struct mychan *mc, const char *name);

void set_global_template_flags(const char *name, unsigned int flags);
unsigned int get_global_template_flags(const char *name);
void clear_global_template_flags(void);
void fix_global_template_flags(void);

extern mowgli_patricia_t *global_template_dict;

#endif /* !ATHEME_INC_TEMPLATE_H */
