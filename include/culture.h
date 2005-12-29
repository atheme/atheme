/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Message translation stuff.
 * 
 * $Id: culture.h 4267 2005-12-29 01:26:21Z nenolod $
 */

#ifndef CULTURE_H
#define CULTURE_H

struct translation_
{
	char *name;
	char *replacement;
};

typedef struct translation_ translation_t;

E char *translation_get(char *name);
E void translation_create(char *str, char *trans);
E void translation_destroy(char *str);

#endif
