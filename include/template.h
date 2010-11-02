/*
 * Copyright (C) 2005 Jilles Tjoelker, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Predefined flags collections
 *
 */

#ifndef TEMPLATE_H
#define TEMPLATE_H

E const char *getitem(const char *str, const char *name);
E unsigned int get_template_flags(mychan_t *mc, const char *name);

E void set_global_template_flags(const char *name, unsigned int flags);
E unsigned int get_global_template_flags(const char *name);
E void clear_global_template_flags(void);

#endif /* TEMPLATE_H */

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
