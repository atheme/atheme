/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Message translation stuff.
 * 
 */

#ifndef CULTURE_H
#define CULTURE_H

E const char *translation_get(const char *name);
E void itranslation_create(const char *str, const char *trans);
E void itranslation_destroy(const char *str);
E void translation_create(const char *str, const char *trans);
E void translation_destroy(const char *str);
E void translation_init(void);

typedef struct language_ language_t;

E language_t *language_add(const char *name);
E language_t *language_find(const char *name);
E const char *language_names(void);
E const char *language_get_name(const language_t *lang);
E const char *language_get_real_name(const language_t *lang);
E bool language_is_valid(const language_t *lang);
E void language_set_active(language_t *lang);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
