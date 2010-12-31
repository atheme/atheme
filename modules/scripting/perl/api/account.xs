# Atheme::Account inherits Atheme::Entity, which inherits
# Atheme::Object (or will).
#

MODULE = Atheme			PACKAGE = Atheme::Entity

Atheme_Entity
find(SV * package, const char * entityname)
CODE:
	RETVAL = myentity_find(entityname);
OUTPUT:
	RETVAL

const char *
name(Atheme_Entity self)
CODE:
	RETVAL = self->name;
OUTPUT:
	RETVAL



MODULE = Atheme			PACKAGE = Atheme::Account

Atheme_Account
find(SV * package, const char * accountname)
CODE:
	RETVAL = myuser_find(accountname);
OUTPUT:
	RETVAL

const char *
email(Atheme_Account self)
CODE:
	RETVAL = self->email;
OUTPUT:
	RETVAL


