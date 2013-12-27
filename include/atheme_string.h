/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * String stuff.
 *
 */

#ifndef ATHEME_STRING_H
#define ATHEME_STRING_H

E void strip(char *line);
E void strip_ctrl(char *line);

#ifndef HAVE_STRTOK_R
E char *strtok_r(char *s, const char *delim, char **lasts);
#endif

#ifndef HAVE_STRCASESTR
E char *strcasestr(char *s, const char *find);
#endif

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
