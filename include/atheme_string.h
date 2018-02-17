/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * String stuff.
 */

#ifndef ATHEME_STRING_H
#define ATHEME_STRING_H

extern void strip(char *line);
extern void strip_ctrl(char *line);

#ifndef HAVE_STRTOK_R
extern char *strtok_r(char *s, const char *delim, char **lasts);
#endif

#ifndef HAVE_STRCASESTR
extern char *strcasestr(char *s, const char *find);
#endif

#endif
