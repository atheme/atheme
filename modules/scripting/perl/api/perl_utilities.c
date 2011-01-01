#include "atheme_perl.h"

static void (*real_register_object_reference)(SV *) = NULL;
static void (*real_invalidate_object_references)(void) = NULL;

SV * bless_pointer_to_package(void *data, const char *package)
{
	SV *ret = newSV(0);
	sv_setref_pv(ret, package, data);

	/* If this function is being called, then the XS method in question
	 * is declared as returning SV*, which means the magic typemap code
	 * doesn't get called, and we have to do this manually.
	 */
	register_object_reference(ret);
	return ret;
}

void register_object_reference(SV *sv)
{
	if (real_register_object_reference == NULL)
	{
		real_register_object_reference = module_locate_symbol(PERL_MODULE_NAME, "register_object_reference");
		if (real_register_object_reference == NULL)
		{
			dTHX;
			Perl_croak(aTHX_ "Couldn't locate symbol register_object_reference in " PERL_MODULE_NAME);
		}
	}
	real_register_object_reference(sv);
}

void invalidate_object_references(void)
{
	if (real_invalidate_object_references == NULL)
	{
		real_invalidate_object_references = module_locate_symbol(PERL_MODULE_NAME,
									"invalidate_object_references");
		if (real_invalidate_object_references == NULL)
		{
			dTHX;
			Perl_croak(aTHX_ "Couldn't locate symbol invalidate_object_references in " PERL_MODULE_NAME);
		}
	}
	real_invalidate_object_references();
}
