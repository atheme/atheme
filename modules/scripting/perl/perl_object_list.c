#include "api/atheme_perl.h"

static mowgli_list_t * perl_object_references = NULL;

void register_object_reference(SV * sv)
{
	dTHX;

	if (!sv_isobject(sv))
		Perl_croak(aTHX_ "Attempted to register an object reference that isn't");

	if (perl_object_references == NULL)
		perl_object_references = mowgli_list_create();

	mowgli_node_add(SvREFCNT_inc(sv), mowgli_node_create(), perl_object_references);
}

void invalidate_object_references(void)
{
	mowgli_node_t *n;

	if (perl_object_references == NULL)
		return;

	while ((n = perl_object_references->head) != NULL)
	{
		SV *sv = n->data;
		SV *ref = SvRV(sv);
		if (!sv_isobject(sv))
			slog(LG_ERROR, "invalidate_object_references: found object reference that isn't");
		else
			SvIVX(ref) = invalid_object_pointer;

		SvREFCNT_dec(sv);
		mowgli_node_delete(n, perl_object_references);
		mowgli_node_free(n);
	}
}

void free_object_list(void)
{
	mowgli_node_t *n;

	if (perl_object_references == NULL)
		return;

	while ((n = perl_object_references->head) != NULL)
	{
		SV *sv = n->data;
		SvREFCNT_dec(sv);
		mowgli_node_delete(n, perl_object_references);
		mowgli_node_free(n);
	}

	mowgli_list_free(perl_object_references);
	perl_object_references = NULL;
}


