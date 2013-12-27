#ifndef ATHEME_PERL_H
#define ATHEME_PERL_H

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#undef _

#include "atheme.h"

struct perl_command_ {
	command_t command;
	SV * handler;
	SV * help_func;
};

typedef struct perl_command_ perl_command_t;

void perl_command_handler(sourceinfo_t *si, const int parc, char **parv);
void perl_command_help_func(sourceinfo_t *si, const char *subcmd);

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

struct perl_list_ {
	mowgli_list_t *list;
	const char *package_name;
};

typedef struct perl_list_ perl_list_t;

static inline perl_list_t * perl_list_create(mowgli_list_t *list, const char *package)
{
	perl_list_t *ret = smalloc(sizeof(perl_list_t));
	ret->list = list;
	ret->package_name = sstrdup(package);
	return ret;
}

/*
 * Utility functions for ease of XS coding
 */

SV * bless_pointer_to_package(void *data, const char *package);

#endif
