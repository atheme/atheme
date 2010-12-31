# Wrapper around a mowgli_list_t to implement a tied array.

MODULE = Atheme			PACKAGE = Atheme::Internal::List

SV *
FETCH(Atheme_Internal_List self, int index)
CODE:
	void *data = mowgli_node_nth_data(self->list, index);
	RETVAL = bless_pointer_to_package(data, self->package_name);
OUTPUT:
	RETVAL

void
STORE(Atheme_Internal_List self, int index, SV *value)
CODE:
	Perl_croak(aTHX_ "Direct modification of lists not supported");

int
FETCHSIZE(Atheme_Internal_List self)
CODE:
	RETVAL = self->list->count;
OUTPUT:
	RETVAL

void
STORESIZE(Atheme_Internal_List self, int count)
CODE:
	Perl_croak(aTHX_ "Direct modification of lists not supported");

void
DELETE(Atheme_Internal_List self)
CODE:
	free((void*)self->package_name);
	free(self);
