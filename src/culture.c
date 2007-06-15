/*
 * atheme-services: A collection of minimalist IRC services   
 * culture.c: Translation framework.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)           
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"

dictionary_tree_t *itranslation_tree; /* internal translations, userserv/nickserv etc */
dictionary_tree_t *translation_tree; /* language translations */

/*
 * translation_init()
 *
 * Initializes the translation DTrees.
 *
 * Inputs:
 *     - none
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - on success, nothing
 *     - on failure, the program will abort
 */
void translation_init(void)
{
	itranslation_tree = dictionary_create("itranslation_tree", HASH_ITRANS, strcmp);
	translation_tree = dictionary_create("translation_tree", HASH_TRANS, strcmp);
}

/*
 * translation_get(const char *str)
 *
 * Inputs:
 *     - string to get translation for
 *
 * Outputs:
 *     - translated string, or the original string if no translation
 *       was found
 *
 * Side Effects:
 *     - none
 */
const char *translation_get(const char *str)
{
	translation_t *t;

	/* See if an internal substitution is present. */
	if ((t = dictionary_retrieve(itranslation_tree, str)) != NULL)
		str = t->replacement;

	if ((t = dictionary_retrieve(translation_tree, str)) != NULL)
		return t->replacement;

	return str;
}

/*
 * itranslation_create(char *str, char *trans)
 *
 * Inputs:
 *     - string to translate, translation of the string
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - a new translation is added to the cache
 */
void itranslation_create(char *str, char *trans)
{
	translation_t *t = smalloc(sizeof(translation_t));

	t->name = sstrdup(str);
	t->replacement = sstrdup(trans);

	dictionary_add(itranslation_tree, t->name, t);
}

/*
 * itranslation_destroy(char *str)
 *
 * Inputs:
 *     - string to remove from the translation cache
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - a string is removed from the translation cache
 */
void itranslation_destroy(char *str)
{
	translation_t *t = dictionary_delete(itranslation_tree, str);

	if (t == NULL)
		return;

	free(t->name);
	free(t->replacement);
	free(t);
}

/*
 * translation_create(char *str, char *trans)
 *
 * Inputs:
 *     - string to translate, translation of the string
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - a new translation is added to the cache
 */
void translation_create(char *str, char *trans)
{
	char buf[BUFSIZE];
	translation_t *t = smalloc(sizeof(translation_t));

	strlcpy(buf, str, BUFSIZE);
	replace(buf, BUFSIZE, "\\2", "\2");

	t->name = sstrdup(buf);

	strlcpy(buf, trans, BUFSIZE);
	replace(buf, BUFSIZE, "\\2", "\2");

	t->replacement = sstrdup(buf);

	dictionary_add(translation_tree, t->name, t);
}

/*
 * translation_destroy(char *str)
 *
 * Inputs:
 *     - string to remove from the translation cache
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - a string is removed from the translation cache
 */
void translation_destroy(char *str)
{
	translation_t *t = dictionary_delete(translation_tree, str);

	if (t == NULL)
		return;

	free(t->name);
	free(t->replacement);
	free(t);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
