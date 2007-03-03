/*
 * Copyright (C) 2005 Jilles Tjoelker, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Predefined flags collections
 *
 * $Id: template.h 7771 2007-03-03 12:46:36Z pippijn $
 */

#ifndef TEMPLATE_H
#define TEMPLATE_H

E char *getitem(char *str, char *name);
E uint32_t get_template_flags(mychan_t *mc, char *name);

#endif /* TEMPLATE_H */

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
