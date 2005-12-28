/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Message translation stuff.
 * 
 * $Id: culture.h 4259 2005-12-28 22:32:35Z nenolod $
 */

#ifndef CULTURE_H
#define CULTURE_H

struct translation_
{
	char *name;
	char *replacement;
};

typedef struct translation_ translation_t;

extern char *translation_get(char *name);

#endif
