MODULE = Atheme			PACKAGE = Atheme::Object

const char *
name(Atheme_Object self)
CODE:
	RETVAL = "";
OUTPUT:
	RETVAL

Atheme_Object_MetadataHash
metadata(Atheme_Object self)
CODE:
	RETVAL = self;
OUTPUT:
	RETVAL
