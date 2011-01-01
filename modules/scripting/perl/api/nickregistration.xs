MODULE = Atheme			PACKAGE = Atheme::NickRegistration

Atheme_NickRegistration
find(SV * package, const char *name)
CODE:
	RETVAL = mynick_find(name);
OUTPUT:
	RETVAL

const char *
nick(Atheme_NickRegistration self)
CODE:
	RETVAL = self->nick;
OUTPUT:
	RETVAL

Atheme_Account
owner(Atheme_NickRegistration self)
CODE:
	RETVAL = self->owner;
OUTPUT:
	RETVAL

