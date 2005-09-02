/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Message translation stuff.
 * 
 * $Id: culture.h 1374 2005-08-01 08:57:17Z nenolod $
 */

#ifndef CULTURE_H
#define CULTURE_H

struct translation_
{
	char *name;
	char *standard;
	char *replacement;
};

typedef struct translation_ translation_t;

extern char *form_str(char *name);
extern char *new_msg(char *name, char *standard);

#endif
