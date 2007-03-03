/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Message translation stuff.
 * 
 * $Id: culture.h 7779 2007-03-03 13:55:42Z pippijn $
 */

#ifndef CULTURE_H
#define CULTURE_H

struct translation_
{
	char *name;
	char *replacement;
};

typedef struct translation_ translation_t;

E const char *translation_get(const char *name);
E void itranslation_create(char *str, char *trans);
E void itranslation_destroy(char *str);
E void translation_create(char *str, char *trans);
E void translation_destroy(char *str);
E void translation_init(void);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
