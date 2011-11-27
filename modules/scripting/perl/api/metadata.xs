MODULE = Atheme			PACKAGE = Atheme::Object::MetadataHash

Atheme_Object_MetadataHash
TIEHASH(SV * self, Atheme_Object object)
CODE:
	/* Underlying pointer is just the same object, blessed into a different package. */
	RETVAL = object;
OUTPUT:
	RETVAL

const char *
FETCH(Atheme_Object_MetadataHash object, const char * key)
CODE:
	metadata_t *md = metadata_find(object, key);
	if (md == NULL)
		XSRETURN_UNDEF;
	RETVAL = md->value;
OUTPUT:
	RETVAL

void
STORE(Atheme_Object_MetadataHash object, const char * key, const char * value)
CODE:
	metadata_add(object, key, value);

void
DELETE(Atheme_Object_MetadataHash object, const char * key)
CODE:
	metadata_delete(object, key);

void
CLEAR(Atheme_Object_MetadataHash object)
CODE:
	metadata_delete_all(object);

bool
EXISTS(Atheme_Object_MetadataHash object, const char * key)
CODE:
	RETVAL = (metadata_find(object, key) != NULL);
OUTPUT:
	RETVAL

const char *
FIRSTKEY(Atheme_Object_MetadataHash object)
CODE:
	XSRETURN_UNDEF;
	RETVAL = NULL;
OUTPUT:
	RETVAL

const char *
NEXTKEY(Atheme_Object_MetadataHash object, const char *lastkey)
CODE:
	XSRETURN_UNDEF;
	RETVAL = NULL;
OUTPUT:
	RETVAL

int
SCALAR(Atheme_Object_MetadataHash object)
CODE:
	RETVAL = 0;
OUTPUT:
	RETVAL


