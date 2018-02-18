/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Message translation stuff.
 */

#ifndef CULTURE_H
#define CULTURE_H

struct language
{
	char *name;
	unsigned int flags; /* LANG_* */
	mowgli_node_t node;
};

extern const char *translation_get(const char *name);
extern void itranslation_create(const char *str, const char *trans);
extern void itranslation_destroy(const char *str);
extern void translation_create(const char *str, const char *trans);
extern void translation_destroy(const char *str);
extern void translation_init(void);

extern struct language *language_add(const char *name);
extern struct language *language_find(const char *name);
extern const char *language_names(void);
extern const char *language_get_name(const struct language *lang);
extern const char *language_get_real_name(const struct language *lang);
extern bool language_is_valid(const struct language *lang);
extern void language_set_active(struct language *lang);

#endif
