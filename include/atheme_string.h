/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * String stuff.
 *
 */

#ifndef __CLAROSTRING
#define __CLAROSTRING

E void strip(char *line);

#ifdef _WIN32
# define strtok_r strtok_s
#endif

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
