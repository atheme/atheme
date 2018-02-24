/*
 * Copyright (C) 2005 Jilles Tjoelker, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Predefined flags collections
 */

#ifndef TEMPLATE_H
#define TEMPLATE_H

struct default_template
{
	unsigned int flags;
};

const char *getitem(const char *str, const char *name);
unsigned int get_template_flags(struct mychan *mc, const char *name);

void set_global_template_flags(const char *name, unsigned int flags);
unsigned int get_global_template_flags(const char *name);
void clear_global_template_flags(void);
void fix_global_template_flags(void);

extern mowgli_patricia_t *global_template_dict;

#endif /* TEMPLATE_H */
