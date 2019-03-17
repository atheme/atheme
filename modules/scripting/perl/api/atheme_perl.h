/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010-2013 Atheme Project (http://atheme.org/)
 */

#ifndef ATHEME_MOD_SCRIPTING_PERL_API_ATHEME_PERL_H
#define ATHEME_MOD_SCRIPTING_PERL_API_ATHEME_PERL_H 1

#include <atheme.h>

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

struct perl_command
{
	struct command command;
	SV * handler;
	SV * help_func;
};

void perl_command_handler(struct sourceinfo *si, const int parc, char **parv);
void perl_command_help_func(struct sourceinfo *si, const char *subcmd);

#define PERL_MODULE_NAME "scripting/perl"
static const IV invalid_object_pointer = -1;

void register_object_reference(SV *sv);
void invalidate_object_references(void);
void free_object_list(void);

/*
 * Wrapper objects for collections.
 *
 * These objects hold a reference to the underlying collection, along with
 * knowledge of which Perl package to bless the contents into.
 */

struct perl_list
{
	mowgli_list_t *list;
	const char *package_name;
};

static inline struct perl_list * ATHEME_FATTR_MALLOC
perl_list_create(mowgli_list_t *list, const char *package)
{
	struct perl_list *ret = smalloc(sizeof(struct perl_list));
	ret->list = list;
	ret->package_name = sstrdup(package);
	return ret;
}

/*
 * Utility functions for ease of XS coding
 */

SV * bless_pointer_to_package(void *data, const char *package);

#endif /* !ATHEME_MOD_SCRIPTING_PERL_API_ATHEME_PERL_H */
