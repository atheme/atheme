/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * String stuff.
 */

#ifndef ATHEME_INC_STRING_H
#define ATHEME_INC_STRING_H 1

#include <atheme/stdheaders.h>

void strip(char *line);
void strip_ctrl(char *line);

#ifndef HAVE_STRTOK_R
char *strtok_r(char *s, const char *delim, char **lasts);
#endif

#ifndef HAVE_STRCASESTR
char *strcasestr(char *s, const char *find);
#endif

#endif /* !ATHEME_INC_STRING_H */
