/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * String stuff.
 *
 * $Id: atheme_string.h 7771 2007-03-03 12:46:36Z pippijn $
 */

#ifndef __CLAROSTRING
#define __CLAROSTRING

#ifndef HAVE_STRLCAT
E size_t strlcat(char *dest, const char *src, size_t count);
#endif
#ifndef HAVE_STRLCPY
E size_t strlcpy(char *dest, const char *src, size_t count);
#endif

E void strip(char *line);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
