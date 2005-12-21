/*
 * Copyright (C) 2005 Jilles Tjoelker, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Predefined flags collections
 *
 * $Id: template.h 4175 2005-12-21 19:23:17Z jilles $
 */

#ifndef TEMPLATE_H
#define TEMPLATE_H

E char *getitem(char *str, char *name);
E uint32_t get_template_flags(mychan_t *mc, char *name);

#endif /* TEMPLATE_H */
