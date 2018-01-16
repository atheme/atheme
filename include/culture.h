/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Message translation stuff.
 *
 */

#ifndef CULTURE_H
#define CULTURE_H

extern const char *translation_get(const char *name);
extern void itranslation_create(const char *str, const char *trans);
extern void itranslation_destroy(const char *str);
extern void translation_create(const char *str, const char *trans);
extern void translation_destroy(const char *str);
extern void translation_init(void);

typedef struct language_ language_t;

extern language_t *language_add(const char *name);
extern language_t *language_find(const char *name);
extern const char *language_names(void);
extern const char *language_get_name(const language_t *lang);
extern const char *language_get_real_name(const language_t *lang);
extern bool language_is_valid(const language_t *lang);
extern void language_set_active(language_t *lang);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
