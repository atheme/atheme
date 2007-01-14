/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * String stuff.
 *
 * $Id: atheme_string.h 7467 2007-01-14 03:25:42Z nenolod $
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
