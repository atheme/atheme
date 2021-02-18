/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * Message translation stuff.
 */

#ifndef ATHEME_INC_CULTURE_H
#define ATHEME_INC_CULTURE_H 1

#include <atheme/stdheaders.h>

struct language
{
	char *          name;
	unsigned int    flags;  // LANG_*
	mowgli_node_t   node;
};

struct translation
{
	char *          name;
	char *          replacement;
};

const char *translation_get(const char *name);
void itranslation_create(const char *str, const char *trans);
void itranslation_destroy(const char *str);
void translation_create(const char *str, const char *trans);
void translation_destroy(const char *str);
void translation_init(void);

struct language *language_add(const char *name);
struct language *language_find(const char *name);
const char *language_names(void);
const char *language_get_name(const struct language *lang);
const char *language_get_real_name(const struct language *lang);
bool language_is_valid(const struct language *lang);
void language_set_active(struct language *lang);

bool languages_get_available(void);
void languages_set_available(bool);

#endif /* !ATHEME_INC_CULTURE_H */
